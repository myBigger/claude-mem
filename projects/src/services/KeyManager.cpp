#include "services/KeyManager.h"
#include "services/CryptoService.h"
#include "utils/PlatformUtils.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QRandomGenerator>
#include <QDebug>

KeyManager& KeyManager::instance() {
    static KeyManager km;
    return km;
}

QString KeyManager_configPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base + "/config");
    return base + "/config/master.key.enc";
}

bool KeyManager::needsSetup() const {
    return !QFile::exists(KeyManager_configPath());
}

void KeyManager::deriveKeyFromPIN(const QString& pin, const QByteArray& salt,
                                  QByteArray& outKey, QByteArray& outIV) {
    // PIN + salt 做 SHA-256 派生得到 AES 密钥
    QByteArray input = (pin + QString::fromUtf8(salt)).toUtf8();
    QByteArray keyHash = CryptoService::instance().sha256(input);
    outKey = keyHash; // 32字节，AES-256 密钥

    // IV = PIN 的 SHA-256 再取后16字节
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(pin.toUtf8());
    hash.addData(salt);
    outIV = hash.result().left(16);
}

bool KeyManager::setup(const QString& pin) {
    if (pin.length() < 6) return false;

    // 1. 生成随机盐
    QByteArray salt;
    salt.resize(32);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 32; ++i) salt[i] = rng->generate() % 256;

    // 2. 生成随机主密钥 AES-256
    QByteArray masterKey, iv;
    CryptoService::instance().generateAESKey(masterKey, iv);
    // generateAESKey 返回的是原始 AES key (32字节) + IV (16字节) 拼接
    // 分离：前32字节是key，后16字节是iv
    QByteArray aesKey = masterKey.left(32);
    QByteArray aesIV  = masterKey.mid(32, 16);

    // 3. 用 PIN 派生密钥加密主密钥
    QByteArray derivedKey, derivedIV;
    deriveKeyFromPIN(pin, salt, derivedKey, derivedIV);
    QByteArray encMasterKey = CryptoService::instance().encryptAES(masterKey, derivedKey, derivedIV);

    // 4. 加密一个固定验证串，用于校验 PIN 是否正确
    const QString kVerifyText = QString::fromUtf8("PrisonSIS_DB_KEY_OK_v1");
    QByteArray encVerify = CryptoService::instance().encryptAES(
        kVerifyText.toUtf8(), derivedKey, derivedIV);

    // 5. 保存到文件
    QJsonObject obj;
    obj["version"] = 1;
    obj["salt"]    = QString::fromUtf8(salt.toBase64());
    obj["verification"] = QString::fromUtf8(encVerify.toBase64());
    obj["encryptedKey"] = QString::fromUtf8(encMasterKey.toBase64());
    obj["iv"]       = QString::fromUtf8(aesIV.toBase64());

    QByteArray jsonData = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QFile f(KeyManager_configPath());
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(jsonData);
    f.close();

    // 6. 用同一 PIN 调用 unlock() 做一次验证，确保能解锁成功
    //    如果验证失败说明写入有问题，此时删除文件并返回 false
    if (!unlock(pin)) {
        qWarning() << "[KeyManager] setup 后 unlock 验证失败，删除配置文件";
        QFile::remove(KeyManager_configPath());
        return false;
    }

    return true;
}

bool KeyManager::unlock(const QString& pin) {
    QFile f(KeyManager_configPath());
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray jsonData = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonObject obj = QJsonDocument::fromJson(jsonData, &err).object();
    if (err.error != QJsonParseError::NoError) return false;

    QByteArray salt     = QByteArray::fromBase64(obj["salt"].toString().toUtf8());
    QByteArray encVerify= QByteArray::fromBase64(obj["verification"].toString().toUtf8());
    QByteArray encKey   = QByteArray::fromBase64(obj["encryptedKey"].toString().toUtf8());

    // 从 PIN 派生密钥（仅需 key，IV 从密文内嵌提取）
    QByteArray derivedKey, derivedIV;
    deriveKeyFromPIN(pin, salt, derivedKey, derivedIV);

    // 校验 PIN：尝试解密验证串（密文已内嵌 IV，用两参数版本自动提取）
    QByteArray decVerify = CryptoService::instance().decryptAES(encVerify, derivedKey);
    if (decVerify != "PrisonSIS_DB_KEY_OK_v1") {
        qWarning() << "[KeyManager] PIN 错误，解锁失败";
        return false;
    }

    // PIN 正确，解密主密钥
    QByteArray decKey = CryptoService::instance().decryptAES(encKey, derivedKey);
    if (decKey.isEmpty()) return false;

    m_masterKey = decKey;
    return true;
}

void KeyManager::lock() {
    // 使用跨平台安全内存清零，防止编译器优化掉清零操作
    // Windows: SecureZeroMemory
    // Linux/macOS: volatile 指针防止优化
    PlatformUtils::secureZeroMemory(m_masterKey);
    m_masterKey.clear();
}

bool KeyManager::changePIN(const QString& oldPin, const QString& newPin) {
    if (!isUnlocked()) return false;
    if (newPin.length() < 6) return false;

    // 读取当前加密文件（用旧 PIN 派生密钥解密不重加密，只重新派生新密钥）
    // 但需要先用 oldPin 解密验证，再重新用 newPin 加密
    // 这里简化：直接重新 setup，生成新的盐和主密钥副本
    // 更安全的做法是：用新 PIN 重新加密 m_masterKey（但 m_masterKey 已是明文）

    // 读取旧文件获取已加密的主密钥
    QFile f(KeyManager_configPath());
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray jsonData = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonObject obj = QJsonDocument::fromJson(jsonData, &err).object();
    if (err.error != QJsonParseError::NoError) return false;

    QByteArray oldSalt     = QByteArray::fromBase64(obj["salt"].toString().toUtf8());
    QByteArray encVerifyOld= QByteArray::fromBase64(obj["verification"].toString().toUtf8());
    QByteArray encKeyOld   = QByteArray::fromBase64(obj["encryptedKey"].toString().toUtf8());

    // 验证旧 PIN（密文已内嵌 IV，用两参数版本自动提取）
    QByteArray oldDerivedKey, oldDerivedIV;
    deriveKeyFromPIN(oldPin, oldSalt, oldDerivedKey, oldDerivedIV);
    QByteArray decVerify = CryptoService::instance().decryptAES(encVerifyOld, oldDerivedKey);
    if (decVerify != "PrisonSIS_DB_KEY_OK_v1") return false;

    // 解密主密钥（用旧 PIN）
    QByteArray masterKey = CryptoService::instance().decryptAES(encKeyOld, oldDerivedKey);
    if (masterKey.isEmpty()) return false;

    // 用新 PIN 重新加密并保存
    QByteArray newSalt;
    newSalt.resize(32);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 32; ++i) newSalt[i] = rng->generate() % 256;

    QByteArray newDerivedKey, newDerivedIV;
    deriveKeyFromPIN(newPin, newSalt, newDerivedKey, newDerivedIV);

    QByteArray encMasterKey = CryptoService::instance().encryptAES(masterKey, newDerivedKey, newDerivedIV);
    const QString kVerifyText = QString::fromUtf8("PrisonSIS_DB_KEY_OK_v1");
    QByteArray encVerify = CryptoService::instance().encryptAES(kVerifyText.toUtf8(), newDerivedKey, newDerivedIV);

    // 生成新 AES IV（从 masterKey 派生）
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(newPin.toUtf8());
    hash.addData(newSalt);
    QByteArray newIV = hash.result().left(16);

    QJsonObject newObj;
    newObj["version"] = 1;
    newObj["salt"]    = QString::fromUtf8(newSalt.toBase64());
    newObj["verification"] = QString::fromUtf8(encVerify.toBase64());
    newObj["encryptedKey"] = QString::fromUtf8(encMasterKey.toBase64());
    newObj["iv"]       = QString::fromUtf8(newIV.toBase64());

    QByteArray newJson = QJsonDocument(newObj).toJson(QJsonDocument::Compact);
    QFile out(KeyManager_configPath());
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(newJson);
    out.close();

    return true;
}

QByteArray KeyManager::encryptField(const QByteArray& plaintext) {
    if (!isUnlocked() || plaintext.isEmpty()) return plaintext;

    // 每次用随机 IV + m_masterKey 加密，实现语义安全
    // encryptAES 返回格式：base64(iv || cipher)，decryptAES 自动从中提取 IV
    QByteArray iv;
    iv.resize(16);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 16; ++i) iv[i] = rng->generate() % 256;
    return CryptoService::instance().encryptAES(plaintext, m_masterKey, iv);
}

QByteArray KeyManager::decryptField(const QByteArray& ciphertext) {
    if (!isUnlocked() || ciphertext.isEmpty()) return ciphertext;
    // decryptAES(ciphertext, key) 会从 ciphertext Base64 解码后提取前16字节作为 IV
    return CryptoService::instance().decryptAES(ciphertext, m_masterKey);
}

QString KeyManager::decryptContent(const QString& fieldValue) {
    if (fieldValue.isEmpty()) return fieldValue;
    if (!instance().isUnlocked()) return fieldValue; // 未解锁，降级明文
    QByteArray raw = fieldValue.toUtf8();
    QByteArray dec = instance().decryptField(raw);
    if (!dec.isEmpty() && dec != raw) {
        return QString::fromUtf8(dec);
    }
    return fieldValue; // 非加密内容或解密失败，返回原文
}
