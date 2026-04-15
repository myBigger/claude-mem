#ifndef CRYPTOSERVICE_H
#define CRYPTOSERVICE_H

#include <QString>
#include <QByteArray>
#include <QMap>

/**
 * CryptoService — OpenSSL 加密服务层
 *
 * 提供：
 * - AES-256-CBC 文件加密/解密
 * - RSA-2048 密钥对生成
 * - SHA-256 哈希计算
 * - RSA-2048 数字签名/验签
 */
class CryptoService {
public:
    static CryptoService& instance();

    // === AES-256-CBC ===
    // 生成随机 AES 密钥（32字节）和 IV（16字节）
    void generateAESKey(QByteArray& outKey, QByteArray& outIV);

    // AES-256 加密（返回 Base64 编码的密文，含IV前缀）
    QByteArray encryptAES(const QByteArray& plaintext,
                          const QByteArray& key,
                          const QByteArray& iv);

    // AES-256 解密（输入 Base64 密文，含IV前缀）
    QByteArray decryptAES(const QByteArray& ciphertextBase64,
                          const QByteArray& key);

    // AES-256 解密（显式传入 IV，不从密文提取）
    QByteArray decryptAES(const QByteArray& ciphertextBase64,
                          const QByteArray& key,
                          const QByteArray& iv);

    // === RSA-2048 ===
    // 生成 RSA-2048 密钥对（返回：key["publicKey"]=PEM, key["privateKey"]=PEM）
    QMap<QString, QString> generateRSAKeyPair();

    // RSA 公钥加密（输入明文，输出 Base64 密文）
    QByteArray rsaEncrypt(const QByteArray& plaintext, const QString& publicKeyPEM);

    // RSA 私钥解密（输入 Base64 密文，输出明文）
    QByteArray rsaDecrypt(const QByteArray& ciphertextBase64, const QString& privateKeyPEM);

    // === SHA-256 ===
    QByteArray sha256(const QByteArray& data);

    // === 数字签名 ===
    // 用私钥对 SHA-256(data) 做签名，返回 Base64 签名值
    QByteArray signData(const QByteArray& data, const QString& privateKeyPEM);

    // 用公钥验证签名，返回 true/false
    bool verifySignature(const QByteArray& data,
                         const QByteArray& signatureBase64,
                         const QString& publicKeyPEM);

    // === 文件操作 ===
    // 加密文件（读取 plaintextPath，写入 ciphertextPath）
    // ciphertext = IV(16字节) + AES加密内容
    bool encryptFile(const QString& plaintextPath,
                      const QString& ciphertextPath,
                      const QByteArray& key);

    // 解密文件（读取 ciphertextPath，写入 plaintextPath）
    bool decryptFile(const QString& ciphertextPath,
                      const QString& plaintextPath,
                      const QByteArray& key);

    // === Base64 ===
    QByteArray base64Encode(const QByteArray& data);
    QByteArray base64Decode(const QByteArray& data);

private:
    explicit CryptoService();
    Q_DISABLE_COPY(CryptoService)
};

#endif // CRYPTOSERVICE_H
