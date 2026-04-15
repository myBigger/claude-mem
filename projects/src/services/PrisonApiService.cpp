#include "services/PrisonApiService.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QDebug>
#include <QRandomGenerator>
#include <QRegularExpression>

// ============================================================
// Base64 编码（标准表，不依赖 Qt 组件）
// ============================================================
static QByteArray base64Encode(const QByteArray& data) {
    static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    QByteArray out;
    int i = 0;
    while (i < (int)data.size()) {
        int b0 = i < (int)data.size() ? (unsigned char)data[i++] : 0;
        int b1 = i < (int)data.size() ? (unsigned char)data[i++] : 0;
        int b2 = i < (int)data.size() ? (unsigned char)data[i++] : 0;
        out.append(tbl[(b0 >> 2) & 63]);
        out.append(tbl[((b0 << 4) | (b1 >> 4)) & 63]);
        out.append(i > (int)data.size() + 1 ? '=' : tbl[((b1 << 2) | (b2 >> 6)) & 63]);
        out.append(i > (int)data.size() ?     '=' : tbl[b2 & 63]);
    }
    return out;
}

// ============================================================
// BigInteger — 基于 QByteArray（大端字节序）
// 不依赖 OpenSSL/第三方库
// ============================================================
class BigInteger {
public:
    BigInteger() : m_data(1, 0) {}                      // 0
    explicit BigInteger(const QByteArray& bytes)        // 从大端字节构造
        : m_data(bytes.isEmpty() ? QByteArray(1, 0) : bytes) { trimLeadingZeros(); }

    bool isZero() const { return m_data.size() == 1 && m_data[0] == 0; }

    // this^exp mod m
    BigInteger powMod(const BigInteger& exp, const BigInteger& mod) const {
        BigInteger base = *this % mod;
        BigInteger result(1);
        BigInteger e = exp;
        while (!e.isZero()) {
            if (e.isOdd()) result = (result * base) % mod;
            base = (base * base) % mod;
            e.div2();
        }
        return result;
    }

    BigInteger operator%(const BigInteger& o) const {
        BigInteger q, r;
        divMod(*this, o, &q, &r);
        return r;
    }

    BigInteger operator*(const BigInteger& o) const {
        if (isZero() || o.isZero()) return BigInteger();
        const BigInteger* a = this;
        const BigInteger* b = &o;
        // 确保 a 是较大的数
        if (m_data.size() < o.m_data.size()) { a = &o; b = this; }
        QByteArray result(a->m_data.size() + b->m_data.size() + 1, 0);
        for (int i = (int)b->m_data.size() - 1; i >= 0; --i) {
            unsigned int carry = 0;
            for (int j = (int)a->m_data.size() - 1; j >= 0; --j) {
                unsigned int cur = (unsigned char)result[i + j + 1]
                    + (unsigned char)a->m_data[j] * (unsigned char)b->m_data[i] + carry;
                result[i + j + 1] = (char)(cur & 0xFF);
                carry = cur >> 8;
            }
            result[i + a->m_data.size()] = (char)(carry & 0xFF);
        }
        BigInteger r;
        r.m_data = result;
        r.trimLeadingZeros();
        return r;
    }

    // 检查奇数（最低字节的最低位为1）
    bool isOdd() const { return !isZero() && (m_data.back() & 1); }

    // 除以2
    void div2() {
        if (isZero()) return;
        for (int i = (int)m_data.size() - 1; i >= 0; --i) {
            unsigned int cur = (unsigned char)m_data[i];
            if (i > 0) {
                unsigned int next = (unsigned char)m_data[i - 1];
                m_data[i] = (char)((cur >> 1) | ((next & 1) ? 0x80 : 0));
            } else {
                m_data[i] = (char)(cur >> 1);
            }
        }
        trimLeadingZeros();
    }

    static void divMod(const BigInteger& num, const BigInteger& den,
                       BigInteger* qout, BigInteger* rout) {
        if (den.isZero()) { if (qout) *qout = BigInteger(); if (rout) *rout = num; return; }
        if (less(num, den)) { if (qout) *qout = BigInteger(); if (rout) *rout = num; return; }

        // 简单长除法（适合演示；如需高效实现可用二分搜索）
        int nLen = num.m_data.size();
        int dLen = den.m_data.size();
        BigInteger q, r;
        q.m_data = QByteArray(nLen, 0);
        r.m_data = QByteArray(1, 0);

        for (int i = nLen - 1; i >= 0; --i) {
            // r = r * 256 + num[i]
            r.shiftLeftAppend((unsigned char)num.m_data[i]);
            // 找商
            unsigned int lo = 0, hi = 256;
            while (lo < hi) {
                unsigned int mid = lo + (hi - lo + 1) / 2;
                BigInteger t = den * mid;
                if (!less(t, r)) hi = mid - 1;
                else lo = mid;
            }
            q.m_data[i] = (char)lo;
            BigInteger sub = den * (int)lo;
            r = r - sub;
        }
        q.trimLeadingZeros();
        r.trimLeadingZeros();
        if (qout) *qout = q;
        if (rout) *rout = r;
    }

    static bool less(const BigInteger& a, const BigInteger& b) {
        if (a.m_data.size() != b.m_data.size()) return a.m_data.size() < b.m_data.size();
        for (int i = (int)a.m_data.size() - 1; i >= 0; --i)
            if ((unsigned char)a.m_data[i] != (unsigned char)b.m_data[i])
                return (unsigned char)a.m_data[i] < (unsigned char)b.m_data[i];
        return false;
    }

    // r = r * 256 + byte
    void shiftLeftAppend(unsigned char byte) {
        for (int i = 0; i < (int)m_data.size(); ++i) {
            unsigned int cur = (unsigned char)(m_data[i]) * 256 + byte;
            m_data[i] = (char)(cur & 0xFF);
            byte = (char)((cur >> 8) & 0xFF);
        }
        if (byte) m_data.append((char)byte);
    }

    BigInteger operator-(const BigInteger& o) const {
        // 假设 this >= o
        QByteArray result = m_data;
        int borrow = 0;
        for (int i = 0; i < (int)m_data.size(); ++i) {
            int diff = (int)(unsigned char)m_data[i] - borrow
                     - (i < (int)o.m_data.size() ? (unsigned char)o.m_data[i] : 0);
            if (diff < 0) { diff += 256; borrow = 1; } else borrow = 0;
            result[i] = (char)diff;
        }
        BigInteger r;
        r.m_data = result;
        r.trimLeadingZeros();
        return r;
    }

    // 支持 BigInteger * int（int 在 0-255）
    BigInteger operator*(int v) const {
        QByteArray result(m_data.size() + 2, 0);
        unsigned int carry = 0;
        for (int i = (int)m_data.size() - 1; i >= 0; --i) {
            unsigned int cur = carry + (unsigned char)m_data[i] * (unsigned int)v;
            result[i + 1] = (char)(cur & 0xFF);
            carry = cur >> 8;
        }
        result[0] = (char)(carry & 0xFF);
        BigInteger r;
        r.m_data = result;
        r.trimLeadingZeros();
        return r;
    }

    QByteArray toBytes() const { return m_data; }

private:
    explicit BigInteger(int v) { // 内部：int 构造
        m_data.clear();
        if (v == 0) { m_data.append(char(0)); return; }
        while (v > 0) { m_data.append(char(v & 0xFF)); v >>= 8; }
        if (m_data.isEmpty() || m_data.back() == 0) {} // ok
    }

    void trimLeadingZeros() {
        while (m_data.size() > 1 && m_data.back() == 0) m_data.chop(1);
    }

    QByteArray m_data; // 大端字节序
};

// DER 编码整数（正数 big-endian，加前导0避免负性）
static QByteArray derInteger(const QByteArray& bytes) {
    QByteArray b = bytes;
    if (b.isEmpty() || (b.back() & 0x80)) b.append(char(0));
    char len = (char)b.size();
    QByteArray hdr;
    if (len < 128) { hdr.append(char(0x02)).append(char(len)); }
    else if (len < 256) { hdr.append(char(0x02)).append(char(0x81)).append(char(len)); }
    else { hdr.append(char(0x02)).append(char(0x82)).append(char(len>>8)).append(char(len&0xFF)); }
    return hdr + b;
}

static QByteArray derSequence(const QByteArray& content) {
    char len = (char)content.size();
    QByteArray hdr;
    if (content.size() < 128) { hdr.append(char(0x30)).append(char(len)); }
    else if (content.size() < 256) { hdr.append(char(0x30)).append(char(0x81)).append(char(content.size())); }
    else { hdr.append(char(0x30)).append(char(0x82)).append(char(content.size()>>8)).append(char(content.size()&0xFF)); }
    return hdr + content;
}

static QByteArray derOid(const QByteArray& oid) {
    QByteArray content;
    int first = oid[0] / 40;
    content.append(char(first));
    content.append(char(oid[0] % 40));
    for (size_t i = 1; i < (size_t)oid.size(); ++i) {
        int v = (unsigned char)oid[i];
        char buf[4]; int n = 0;
        while (v > 0) { buf[n++] = char(v & 0x7F); v >>= 7; }
        for (int k = n - 1; k >= 0; --k) {
            char c = buf[k];
            if (k > 0) c = char(c | 0x80);
            content.append(c);
        }
    }
    char len = (char)content.size();
    QByteArray hdr;
    if (len < 128) hdr.append(char(0x06)).append(char(len));
    else hdr.append(char(0x06)).append(char(0x81)).append(char(len));
    return hdr + content;
}

static QByteArray derBitString(const QByteArray& content) {
    char len = (char)(content.size() + 1);
    QByteArray hdr;
    if (content.size() < 128) hdr.append(char(0x03)).append(char(len));
    else hdr.append(char(0x03)).append(char(0x81)).append(char(len));
    return hdr + QByteArray(1, char(0)) + content;
}

// RSA PKCS1 v1.5 加密: message^e mod n
// modulusHex / exponentHex 为十六进制字符串
static QByteArray rsaEncryptRaw(const QByteArray& message,
                                 const QByteArray& modulusHex,
                                 const QByteArray& exponentHex) {
    // 将十六进制转为字节
    auto hexToBytes = [](const QByteArray& hex) -> QByteArray {
        QByteArray r;
        for (int i = 0; i < hex.size(); i += 2) {
            bool ok; int v = hex.mid(i, 2).toInt(&ok, 16);
            if (ok) r.append((char)v);
        }
        return r;
    };

    QByteArray nBytes = hexToBytes(modulusHex);
    QByteArray eBytes = hexToBytes(exponentHex);
    if (nBytes.isEmpty() || eBytes.isEmpty()) return QByteArray();

    // 构建 DER SubjectPublicKeyInfo (RSA PKCS#1)
    QByteArray nInt = derInteger(nBytes);
    QByteArray eInt = derInteger(eBytes);
    QByteArray rsaSeq = derSequence(nInt + eInt);
    // algorithmIdentifier: 1.2.840.113549.1.1.1 = OID 2.5.29.17
    // RSA PKCS1 OID: 1.2.840.113549.1.1.1
    QByteArray algId = derOid(QByteArray::fromRawData("\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01", 9))
                     + QByteArray::fromRawData("\x05\x00", 2);
    QByteArray spki = derSequence(algId + derBitString(rsaSeq));

    // PKCS1 v1.5 填充: 00 || 02 || PS || 00 || D
    int keyLen = nBytes.size();
    int psLen = keyLen - message.size() - 3;
    if (psLen < 8) return QByteArray();
    QByteArray EB(keyLen, 0);
    EB[0] = 0x00; EB[1] = 0x02;
    for (int i = 0; i < psLen; ++i) EB[2 + i] = (char)(QRandomGenerator::global()->bounded(1, 256));
    EB[2 + psLen] = 0x00;
    for (int i = 0; i < message.size(); ++i) EB[3 + psLen + i] = message[i];

    // RSA: c = m^e mod n
    BigInteger m(EB);
    BigInteger n(modulusHex.isEmpty() ? nBytes : hexToBytes(modulusHex));
    BigInteger e(hexToBytes(exponentHex));
    // 使用 HEX 构造的 BigInteger 需特殊处理
    // 直接用字节构造 m, n, e
    Q_UNUSED(n); Q_UNUSED(e);
    // 重写 powMod 使用字节构造
    BigInteger M(EB);
    BigInteger N(hexToBytes(modulusHex));
    BigInteger E(hexToBytes(exponentHex));
    BigInteger C = M.powMod(E, N);
    QByteArray ct = C.toBytes();
    if (ct.size() < keyLen) ct.prepend(QByteArray(keyLen - ct.size(), 0));
    return ct;
}

// 简化 wrapper：接受十六进制字符串参数
static QByteArray rsaEncryptHex(const QString& messageHex,
                                 const QString& modulusHex,
                                 const QString& exponentHex) {
    // messageHex 为普通字符串的HEX表示
    QByteArray msgBytes;
    for (int i = 0; i < messageHex.size(); i += 2) {
        bool ok; int v = messageHex.mid(i, 2).toInt(&ok, 16);
        if (ok) msgBytes.append((char)v);
    }
    QByteArray result = rsaEncryptRaw(msgBytes, modulusHex.toLatin1(), exponentHex.toLatin1());
    return result;
}

// 直接加密字符串，返回 Base64
static QString rsaEncryptString(const QString& plainText,
                                 const QString& modulusHex,
                                 const QString& exponentHex) {
    if (modulusHex.isEmpty() || exponentHex.isEmpty()) return QString();
    QByteArray pt = plainText.toUtf8();
    QByteArray nH = modulusHex.toLatin1();
    QByteArray eH = exponentHex.toLatin1();
    QByteArray ct = rsaEncryptRaw(pt, nH, eH);
    return base64Encode(ct).constData();
}

// ============================================================
// PrisonApiService 实现
// ============================================================

PrisonApiService& PrisonApiService::instance() {
    static PrisonApiService inst;
    return inst;
}

PrisonApiService::PrisonApiService(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    loadConfig();
}

PrisonApiService::~PrisonApiService() {}

QString PrisonApiService::configFilePath() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    return path + "/api.ini";
}

void PrisonApiService::loadConfig() {
    QSettings s(configFilePath(), QSettings::IniFormat);
    m_casBaseUrl    = s.value("cas/base_url",    "http://192.168.1.100:8080/cas").toString();
    m_casLoginPath  = s.value("cas/login_path",  "/login").toString();
    m_casServiceParam = s.value("cas/service_param","service").toString();
    m_cookieNames = s.value("cas/cookie_names","JSESSIONID").toString().split(',', Qt::SkipEmptyParts);
    m_rsaKeyPath   = s.value("rsa/key_path",    "/pubkey").toString();
    m_rsaModulus   = s.value("rsa/modulus",     "").toString();
    m_rsaExponent  = s.value("rsa/exponent",    "10001").toString();
    m_apiBaseUrl   = s.value("api/base_url",    "http://192.168.1.100:8080/prison-api").toString();
    m_criminalGetPath    = s.value("api/criminal_get",    "/criminal/get").toString();
    m_criminalSearchPath = s.value("api/criminal_search","/criminal/search").toString();
    m_criminalListPath   = s.value("api/criminal_list",  "/criminal/list").toString();
    m_enabled = s.value("api/enabled", false).toBool();
    m_fieldMap["criminal_id"]    = s.value("fields/criminal_id","criminal_id").toString();
    m_fieldMap["name"]            = s.value("fields/name","name").toString();
    m_fieldMap["gender"]          = s.value("fields/gender","gender").toString();
    m_fieldMap["ethnicity"]       = s.value("fields/ethnicity","ethnicity").toString();
    m_fieldMap["birth_date"]      = s.value("fields/birth_date","birth_date").toString();
    m_fieldMap["id_card_number"]  = s.value("fields/id_card_number","id_card_number").toString();
    m_fieldMap["native_place"]    = s.value("fields/native_place","native_place").toString();
    m_fieldMap["education"]       = s.value("fields/education","education").toString();
    m_fieldMap["crime"]           = s.value("fields/crime","crime").toString();
    m_fieldMap["sentence_years"]  = s.value("fields/sentence_years","sentence_years").toString();
    m_fieldMap["sentence_months"] = s.value("fields/sentence_months","sentence_months").toString();
    m_fieldMap["entry_date"]     = s.value("fields/entry_date","entry_date").toString();
    m_fieldMap["original_court"]  = s.value("fields/original_court","original_court").toString();
    m_fieldMap["district"]        = s.value("fields/district","district").toString();
    m_fieldMap["cell"]            = s.value("fields/cell","cell").toString();
    m_fieldMap["crime_type"]      = s.value("fields/crime_type","crime_type").toString();
    m_fieldMap["manage_level"]    = s.value("fields/manage_level","manage_level").toString();
    m_fieldMap["handler_name"]    = s.value("fields/handler_name","handler_name").toString();
    m_fieldMap["photo_url"]       = s.value("fields/photo_url","photo_url").toString();
    m_fieldMap["remark"]          = s.value("fields/remark","remark").toString();
    qDebug() << "[PrisonApi] 配置加载: enabled=" << m_enabled
             << "CAS=" << m_casBaseUrl << "RSA modulus len=" << m_rsaModulus.size();
}

void PrisonApiService::saveConfig() {
    QSettings s(configFilePath(), QSettings::IniFormat);
    s.beginGroup("cas");
    s.setValue("base_url",       m_casBaseUrl);
    s.setValue("login_path",     m_casLoginPath);
    s.setValue("service_param",  m_casServiceParam);
    s.setValue("cookie_names",   m_cookieNames.join(','));
    s.endGroup();
    s.beginGroup("rsa");
    s.setValue("key_path",  m_rsaKeyPath);
    s.setValue("modulus",   m_rsaModulus);
    s.setValue("exponent",  m_rsaExponent);
    s.endGroup();
    s.beginGroup("api");
    s.setValue("base_url",         m_apiBaseUrl);
    s.setValue("criminal_get",     m_criminalGetPath);
    s.setValue("criminal_search",  m_criminalSearchPath);
    s.setValue("criminal_list",    m_criminalListPath);
    s.setValue("enabled",          m_enabled);
    s.endGroup();
}

bool  PrisonApiService::isEnabled()  const { return m_enabled; }
bool  PrisonApiService::isLoggedIn() const { return m_loggedIn; }
QString PrisonApiService::baseUrl()  const { return m_apiBaseUrl; }

void PrisonApiService::logout() {
    m_loggedIn = false;
    m_sessionCookies.clear();
    emit logoutComplete();
}

QString PrisonApiService::encryptByRSA(const QString& plainText) {
    if (m_rsaModulus.isEmpty() || m_rsaExponent.isEmpty()) return QString();
    return rsaEncryptString(plainText, m_rsaModulus, m_rsaExponent);
}

void PrisonApiService::parsePublicKey(const QJsonObject& keyObj) {
    if (keyObj.contains("modulus")) m_rsaModulus = keyObj["modulus"].toString();
    else if (keyObj.contains("n"))   m_rsaModulus = keyObj["n"].toString();
    else if (keyObj.contains("m"))   m_rsaModulus = keyObj["m"].toString();
    if (keyObj.contains("exponent")) m_rsaExponent = keyObj["exponent"].toString();
    else if (keyObj.contains("e"))   m_rsaExponent = keyObj["e"].toString();
    else if (keyObj.contains("pub_key_exp")) m_rsaExponent = keyObj["pub_key_exp"].toString();
    qDebug() << "[PrisonApi] RSA公钥解析完成, modulus=" << m_rsaModulus.left(16) << "...";
}

bool PrisonApiService::parsePublicKeyPEM(const QString& pem) {
    QRegularExpression rx("-----BEGIN[ A-Z0-9]*PUBLIC KEY-----[\r\n ]*(.+)[\r\n ]*-----END[ A-Z0-9]*PUBLIC KEY-----",
                          QRegularExpression::MultilineOption);
    QRegularExpressionMatch m = rx.match(pem.trimmed());
    if (!m.hasMatch()) return false;
    QByteArray der = QByteArray::fromBase64(m.captured(1).trimmed().toLatin1());
    if (der.isEmpty() || (unsigned char)der[0] != 0x30) return false;
    // 找到 bitstring (0x03)
    int i = 0;
    while (i < der.size() && (unsigned char)der[i] != 0x03) ++i;
    if (i >= der.size() - 2) return false;
    int skip = (unsigned char)der[i+1];
    if (skip & 0x80) { // 长格式
        int lenBytes = skip & 0x7F;
        skip = 0;
        for (int k = 0; k < lenBytes; ++k) skip = (skip << 8) | (unsigned char)der[i+2+k];
        i += 2 + lenBytes;
    } else {
        i += 2;
    }
    i += skip; // 跳过空位
    // 现在是 SEQUENCE { n, e }
    i += 2; // 跳过 SEQUENCE tag+len
    int nLen = (unsigned char)der[i]; i += nLen + 1;
    int eLen = (unsigned char)der[i]; i += eLen + 1;
    // 读取 n
    int nStart = i - nLen;
    while (nStart < i && der[nStart] == 0) ++nStart;
    QByteArray nBytes = der.mid(nStart, i - nStart);
    int eStart = i;
    QByteArray eBytes = der.mid(eStart, eLen);

    auto bytesToHex = [](const QByteArray& b)->QString {
        QString r; r.reserve(b.size()*2);
        for (unsigned char c: b) r += QString("%1").arg(c,2,16,QChar('0')).toUpper();
        return r;
    };
    m_rsaModulus  = bytesToHex(nBytes);
    m_rsaExponent = bytesToHex(eBytes);
    qDebug() << "[PrisonApi] PEM解析: modulus=" << m_rsaModulus.left(16) << "...";
    return true;
}

void PrisonApiService::fetchPublicKey() {
    if (m_casBaseUrl.isEmpty()) { emit rsaKeyError("CAS地址未配置"); return; }
    sendRawRequest(m_casBaseUrl + m_rsaKeyPath);
}

void PrisonApiService::initAndLogin(const QString& casBaseUrl,
                                     const QString& username,
                                     const QString& password) {
    if (!casBaseUrl.isEmpty()) m_casBaseUrl = casBaseUrl;
    if (!m_rsaModulus.isEmpty() && !m_rsaExponent.isEmpty()) {
        doLogin(username, password);
    } else {
        connect(this, &PrisonApiService::rsaKeyReady, this,
            [this, username, password]() {
                disconnect(this, &PrisonApiService::rsaKeyReady, this, nullptr);
                doLogin(username, password);
            }, Qt::UniqueConnection);
        fetchPublicKey();
    }
}

void PrisonApiService::doLogin(const QString& username, const QString& password) {
    if (m_rsaModulus.isEmpty() || m_rsaExponent.isEmpty()) {
        emit loginFailed("RSA公钥未就绪");
        return;
    }
    QString encryptedPwd = encryptByRSA(password);
    if (encryptedPwd.isEmpty()) {
        emit loginFailed("密码加密失败");
        return;
    }

    QUrlQuery body;
    body.addQueryItem("username", username);
    body.addQueryItem("password", encryptedPwd);
    body.addQueryItem("lt", "");
    body.addQueryItem("execution", "e1s1");
    body.addQueryItem("_eventId", "submit");
    QByteArray postData = body.toString(QUrl::FullyEncoded).toUtf8();

    qDebug() << "[PrisonApi] CAS登录:" << m_casBaseUrl + m_casLoginPath;
    emit requestStarted();
    QNetworkRequest req(QUrl(m_casBaseUrl + m_casLoginPath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setHeader(QNetworkRequest::UserAgentHeader, "PrisonRecordTool/1.0");
    req.setRawHeader("Referer", m_casBaseUrl.toUtf8());
    QNetworkReply* reply = m_manager->post(req, postData);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        emit requestFinished();
        handleCasLoginReply(reply);
    });
}

void PrisonApiService::handleCasLoginReply(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed("网络错误：" + reply->errorString());
        return;
    }
    saveSessionCookies(reply);
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString location = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().toString();
    QByteArray body = reply->readAll();

    qDebug() << "[PrisonApi] CAS响应: HTTP" << status << "cookies:" << m_sessionCookies.keys();

    if ((status == 302 || status == 303) && !location.isEmpty()
        && !location.contains("login", Qt::CaseInsensitive)
        && !location.contains("error", Qt::CaseInsensitive)) {
        m_loggedIn = true;
        emit loginSuccess();
        flushPendingRequests();
        return;
    }
    if (!m_sessionCookies.isEmpty() && status == 200) {
        QString txt = QString::fromUtf8(body);
        if (!txt.contains("error", Qt::CaseInsensitive) && !txt.contains("登录失败")) {
            m_loggedIn = true;
            emit loginSuccess();
            flushPendingRequests();
            return;
        }
    }
    emit loginFailed(QString("CAS认证失败 HTTP %1，请检查用户名密码或CAS配置").arg(status));
}

void PrisonApiService::saveSessionCookies(QNetworkReply* reply) {
    for (const QByteArray& hdrName : reply->rawHeaderList()) {
        if (hdrName.toLower() == "set-cookie" || hdrName.toLower() == "set-cookie2") {
            QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(reply->rawHeader(hdrName));
            for (const QNetworkCookie& c : cookies) {
                QString name = c.name().constData();
                QString value = c.value().constData();
                if (!name.isEmpty()) {
                    m_sessionCookies[name] = value;
                    qDebug() << "[PrisonApi] Cookie保存:" << name << "=" << value.left(20);
                }
            }
        }
    }
}

void PrisonApiService::fetchById(const QString& criminalId) {
    if (!m_loggedIn) {
        m_pendingRequests.append({"FETCH_BY_ID", criminalId});
        emit fetchError("请先登录");
        return;
    }
    QUrlQuery q;
    q.addQueryItem(m_fieldMap["criminal_id"], criminalId);
    sendRawRequest(m_apiBaseUrl + m_criminalGetPath, q);
}

void PrisonApiService::searchByName(const QString& name) {
    if (!m_loggedIn) {
        m_pendingRequests.append({"SEARCH_BY_NAME", name});
        emit fetchError("请先登录");
        return;
    }
    QUrlQuery q;
    q.addQueryItem("name", name);
    q.addQueryItem("exact", "false");
    sendRawRequest(m_apiBaseUrl + m_criminalSearchPath, q);
}

void PrisonApiService::syncPage(int page, int pageSize) {
    if (!m_loggedIn) {
        m_pendingRequests.append({"SYNC_PAGE", QString("%1:%2").arg(page).arg(pageSize)});
        emit fetchError("请先登录");
        return;
    }
    QUrlQuery q;
    q.addQueryItem("page", QString::number(page));
    q.addQueryItem("page_size", QString::number(pageSize));
    sendRawRequest(m_apiBaseUrl + m_criminalListPath, q);
}

void PrisonApiService::sendRawRequest(const QString& fullUrl,
                                       const QUrlQuery& query) {
    QUrl url(fullUrl);
    if (!query.isEmpty()) url.setQuery(query);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    req.setHeader(QNetworkRequest::UserAgentHeader, "PrisonRecordTool/1.0");
    req.setTransferTimeout(20000);

    if (!m_sessionCookies.isEmpty()) {
        QStringList parts;
        for (auto it = m_sessionCookies.constBegin(); it != m_sessionCookies.constEnd(); ++it)
            parts.append(it.key() + "=" + it.value());
        req.setRawHeader("Cookie", parts.join("; ").toUtf8());
    }

    qDebug() << "[PrisonApi] GET" << url.toString();
    emit requestStarted();
    QNetworkReply* reply = m_manager->get(req);
    connect(reply, &QNetworkReply::finished, this, [=]() { onReplyFinished(reply); });
}

void PrisonApiService::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    emit requestFinished();

    if (reply->error() != QNetworkReply::NoError) {
        QString msg = reply->errorString();
        qWarning() << "[PrisonApi] HTTP错误:" << msg;
        if (reply->url().toString().contains(m_rsaKeyPath)) emit rsaKeyError(msg);
        else emit fetchError("网络错误：" + msg);
        return;
    }

    saveSessionCookies(reply);
    QByteArray raw = reply->readAll();
    QString urlStr = reply->url().toString();

    // 公钥响应
    if (urlStr.contains(m_rsaKeyPath)) {
        QJsonParseError pe;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
        if (pe.error == QJsonParseError::NoError && doc.isObject())
            parsePublicKey(doc.object());
        else if (!parsePublicKeyPEM(QString::fromUtf8(raw)))
            emit rsaKeyError("公钥解析失败");
        else
            emit rsaKeyReady();
        return;
    }

    // 数据响应
    QJsonParseError pe;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &pe);
    if (pe.error != QJsonParseError::NoError) {
        emit fetchError("JSON解析失败：" + pe.errorString());
        return;
    }
    QJsonArray results;
    if (doc.isArray()) results = doc.array();
    else if (doc.isObject()) {
        QJsonObject o = doc.object();
        int code = o.value("code").toInt(-1);
        if (code != 0 && code != 200 && code != 201) {
            emit fetchError("接口错误[" + QString::number(code) + "]：" +
                            o.value("msg").toString(o.value("message").toString("未知错误")));
            return;
        }
        QJsonValue d = o.value("data");
        if (d.isArray()) results = d.toArray();
        else if (d.isObject()) results.append(d.toObject());
    }
    if (results.isEmpty()) { emit fetchError("未查到数据"); return; }
    qDebug() << "[PrisonApi] 查到" << results.size() << "条";
    emit fetchSuccess(results);
}

void PrisonApiService::flushPendingRequests() {
    for (const PendingRequest& req : std::as_const(m_pendingRequests)) {
        if (req.type == "FETCH_BY_ID") fetchById(req.param);
        else if (req.type == "SEARCH_BY_NAME") searchByName(req.param);
        else if (req.type == "SYNC_PAGE") {
            QStringList parts = req.param.split(':');
            syncPage(parts[0].toInt(), parts[1].toInt());
        }
    }
    m_pendingRequests.clear();
}
