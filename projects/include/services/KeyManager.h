#ifndef KEYMANAGER_H
#define KEYMANAGER_H

#include <QString>
#include <QByteArray>

class KeyManagerDialog;

/**
 * KeyManager — 负责数据库内容加密的主密钥管理
 *
 * 设计：
 * - 主密钥（MasterKey）：随机生成的 AES-256 密钥，用于加密数据库敏感字段
 * - 主密钥由用户 PIN 加密存储，PIN 错误则无法解锁
 * - 程序关闭后主密钥从内存消失，需要重新输入 PIN 解锁
 *
 * 存储位置：
 *   ~/.local/share/PrisonSystem/config/master.key.enc
 *
 * 文件格式（JSON）：
 * {
 *   "version": 1,
 *   "salt": "<随机盐，Base64>",          // 用于 PIN → AES-Key 派生
 *   "verification": "<加密的固定字符串, Base64>", // 用于校验 PIN 是否正确
 *   "encryptedKey": "<加密后的 MasterKey, Base64>",
 *   "iv": "<加密 IV, Base64>"
 * }
 */
class KeyManager {
public:
    static KeyManager& instance();

    // 主密钥是否已解锁（内存中持有 MasterKey）
    bool isUnlocked() const { return !m_masterKey.isEmpty(); }

    // 获取当前主密钥（调用前必须确认 isUnlocked() == true）
    QByteArray masterKey() const { return m_masterKey; }

    // 首次使用：检查是否需要初始化
    bool needsSetup() const;

    // 首次设置 PIN：生成主密钥并用 PIN 加密存储
    // 返回 true 表示设置成功
    bool setup(const QString& pin);

    // 用 PIN 解锁主密钥
    // 返回 true 表示 PIN 正确且已解锁
    bool unlock(const QString& pin);

    // 锁定（程序退出或用户主动锁定）
    void lock();

    // 修改 PIN（需要当前已解锁）
    bool changePIN(const QString& oldPin, const QString& newPin);

    // 加密接口（供 DatabaseManager 调用）
    QByteArray encryptField(const QByteArray& plaintext);
    QByteArray decryptField(const QByteArray& ciphertext);

    // 通用字段解密：加密则解密，原始明文则直接返回
    // 用于 records.content 等字段的批量解密
    static QString decryptContent(const QString& fieldValue);

private:
    KeyManager() = default;
    ~KeyManager() { lock(); }
    Q_DISABLE_COPY(KeyManager)

    void deriveKeyFromPIN(const QString& pin, const QByteArray& salt,
                          QByteArray& outKey, QByteArray& outIV);

    QByteArray m_masterKey;  // 解锁后存于内存，lock() 时清空
    QString m_configPath;
};

#endif // KEYMANAGER_H
