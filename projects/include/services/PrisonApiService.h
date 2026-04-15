#ifndef PRISONAPISERVICE_H
#define PRISONAPISERVICE_H
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QString>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QUrlQuery>

/**
 * PrisonApiService — 狱政管理平台 HTTP API 调用服务
 *
 * 完整认证流程（CAS + RSA）：
 *   1. 从 CAS 公钥接口获取 RSA 公钥（MODULUS + EXPONENT）
 *   2. 用公钥加密用户密码（RSA/ECB/PKCS1Padding）
 *   3. 发送加密后的密码至 CAS 登录接口完成认证
 *   4. 保存 CAS 返回的 Session Cookie（JSESSIONID）
 *   5. 后续所有罪犯数据请求携带 Session Cookie
 *
 * 配置文件 ~/.local/share/PrisonSystem/PrisonRecordTool/api.ini
 *   [cas]
 *   enabled        = true
 *   base_url       = http://内网CAS地址/cas
 *   login_path     = /login              （CAS登录接口，POST）
 *   service_param  = service             （Service参数名）
 *   cookie_names   = JSESSIONID          （Session Cookie名称，逗号分隔）
 *
 *   [rsa]
 *   key_path       = /pubkey             （获取RSA公钥的接口，GET）
 *   modulus        = （可选，公钥模数，HEX字符串，不填则从key_path自动获取）
 *   exponent       = （可选，公钥指数，HEX字符串，默认10001）
 *
 *   [api]
 *   base_url       = http://内网API地址/prison-api
 *   criminal_get   = /criminal/get
 *   criminal_search= /criminal/search
 *   criminal_list  = /criminal/list
 */
class PrisonApiService : public QObject {
    Q_OBJECT
public:
    static PrisonApiService& instance();
    ~PrisonApiService();

    // === 配置 ===
    bool isEnabled() const;
    void loadConfig();
    void saveConfig();
    QString baseUrl() const;

    // === CAS 认证 ===
    // 初始化：获取 RSA 公钥（首次或公钥过期时调用）
    void initAndLogin(const QString& casBaseUrl, const QString& username, const QString& password);
    // 仅获取公钥（不解密，用于调试）
    void fetchPublicKey();

    // === 罪犯数据查询（需先完成 CAS 登录）===
    void fetchById(const QString& criminalId);
    void searchByName(const QString& name);
    void syncPage(int page = 1, int pageSize = 20);

    // === 认证状态 ===
    bool isLoggedIn() const;
    void logout();

signals:
    // CAS 认证信号
    void rsaKeyReady();
    void rsaKeyError(const QString& error);
    void loginSuccess();
    void loginFailed(const QString& error);
    void logoutComplete();

    // 数据查询信号
    void fetchSuccess(const QJsonArray& results);
    void fetchError(const QString& error);
    void requestStarted();
    void requestFinished();

private:
    explicit PrisonApiService(QObject* parent = nullptr);
    Q_DISABLE_COPY(PrisonApiService)

    // RSA 工具
    QString encryptByRSA(const QString& plainText);
    void parsePublicKey(const QJsonObject& keyObj);
    bool parsePublicKeyPEM(const QString& pem);

    // 内部请求
    void sendRequest(const QString& fullUrl,
                     const QUrlQuery& query = QUrlQuery(),
                     const QByteArray& postData = QByteArray());
    void sendRawRequest(const QString& fullUrl, const QUrlQuery& query = QUrlQuery());
    void onReplyFinished(QNetworkReply* reply);
    void handleCasLoginReply(QNetworkReply* reply);
    void doLogin(const QString& username, const QString& password);
    void flushPendingRequests();
    void saveSessionCookies(QNetworkReply* reply);
    QString configFilePath() const;

    QNetworkAccessManager* m_manager = nullptr;

    // === CAS 配置 ===
    QString m_casBaseUrl;
    QString m_casLoginPath;
    QString m_casServiceParam;
    QStringList m_cookieNames;

    // === RSA 配置 ===
    QString m_rsaKeyPath;
    QString m_rsaModulus;   // HEX 字符串
    QString m_rsaExponent;  // HEX 字符串，默认 10001

    // === API 配置 ===
    QString m_apiBaseUrl;
    QString m_apiKey;
    QString m_criminalGetPath;
    QString m_criminalSearchPath;
    QString m_criminalListPath;
    bool m_enabled = false;

    // === 认证状态 ===
    bool m_loggedIn = false;
    QMap<QString, QString> m_sessionCookies; // name -> value

    // === 字段映射 ===
    QMap<QString, QString> m_fieldMap;

    // === 待发请求队列（登录前缓存）===
    struct PendingRequest {
        QString type;
        QString param;
        PendingRequest() {}
        PendingRequest(const QString& t, const QString& p) : type(t), param(p) {}
    };
    QList<PendingRequest> m_pendingRequests;
};

#endif // PRISONAPISERVICE_H
