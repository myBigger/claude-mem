#include "services/CryptoService.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

// ===================== Base64（纯手写，无外部依赖）=====================
static const char BASE64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static QByteArray base64Encode(const QByteArray& input) {
    QByteArray output;
    int i = 0;
    while (i < (int)input.size()) {
        int b0 = i < (int)input.size() ? (unsigned char)input[i++] : 0;
        int b1 = i < (int)input.size() ? (unsigned char)input[i++] : 0;
        int b2 = i < (int)input.size() ? (unsigned char)input[i++] : 0;
        output.append(BASE64_TABLE[(b0 >> 2) & 63]);
        output.append(BASE64_TABLE[((b0 << 4) | (b1 >> 4)) & 63]);
        output.append(i > (int)input.size() + 1 ? '=' : BASE64_TABLE[((b1 << 2) | (b2 >> 6)) & 63]);
        output.append(i > (int)input.size() ?     '=' : BASE64_TABLE[b2 & 63]);
    }
    return output;
}

static QByteArray base64Decode(const QByteArray& input) {
    static unsigned char INV[256];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 256; ++i) INV[i] = 255;
        for (int i = 0; i < 64; ++i) INV[(unsigned char)BASE64_TABLE[i]] = i;
        INV['='] = 0;
        init = true;
    }
    QByteArray output;
    for (int i = 0; i < (int)input.size();) {
        int p1 = (i < (int)input.size()) ? INV[(unsigned char)input[i++]] : 0;
        int p2 = (i < (int)input.size()) ? INV[(unsigned char)input[i++]] : 0;
        int p3 = (i < (int)input.size()) ? INV[(unsigned char)input[i++]] : 0;
        int p4 = (i < (int)input.size()) ? INV[(unsigned char)input[i++]] : 0;
        if (p1 == 255 || p2 == 255) break;
        output.append((char)((p1 << 2) | (p2 >> 4)));
        if (p3 < 64) { output.append((char)((p2 << 4) | (p3 >> 2))); }
        if (p4 < 64) { output.append((char)((p3 << 6) | p4)); }
    }
    return output;
}

// ===================== CryptoService 实现 =====================
CryptoService& CryptoService::instance() {
    static CryptoService inst;
    return inst;
}

CryptoService::CryptoService() {
    // OpenSSL 自动加载算法
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

void CryptoService::generateAESKey(QByteArray& outKey, QByteArray& outIV) {
    outKey.resize(32);  // AES-256: 256bits = 32bytes
    outIV.resize(16);  // CBC: 128bits = 16bytes
    RAND_bytes(reinterpret_cast<unsigned char*>(outKey.data()), 32);
    RAND_bytes(reinterpret_cast<unsigned char*>(outIV.data()), 16);
}

QByteArray CryptoService::encryptAES(const QByteArray& plaintext,
                                     const QByteArray& key,
                                     const QByteArray& iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    QByteArray output(plaintext.size() + 32, 0);
    int outLen1 = 0, outLen2 = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                        reinterpret_cast<const unsigned char*>(key.constData()),
                        reinterpret_cast<const unsigned char*>(iv.constData()));
    EVP_EncryptUpdate(ctx,
                        reinterpret_cast<unsigned char*>(output.data()), &outLen1,
                        reinterpret_cast<const unsigned char*>(plaintext.constData()),
                        plaintext.size());
    EVP_EncryptFinal_ex(ctx,
                          reinterpret_cast<unsigned char*>(output.data() + outLen1), &outLen2);
    EVP_CIPHER_CTX_free(ctx);

    output.resize(outLen1 + outLen2);
    // 前面加上 IV
    QByteArray result = iv + output;
    return base64Encode(result);
}

QByteArray CryptoService::decryptAES(const QByteArray& ciphertextBase64,
                                      const QByteArray& key) {
    QByteArray data = base64Decode(ciphertextBase64);
    if (data.size() < 16) return QByteArray();
    QByteArray iv = data.left(16);
    QByteArray cipher = data.mid(16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    QByteArray output(cipher.size() + 32, 0);
    int outLen1 = 0, outLen2 = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                        reinterpret_cast<const unsigned char*>(key.constData()),
                        reinterpret_cast<const unsigned char*>(iv.constData()));
    EVP_DecryptUpdate(ctx,
                        reinterpret_cast<unsigned char*>(output.data()), &outLen1,
                        reinterpret_cast<const unsigned char*>(cipher.constData()),
                        cipher.size());
    EVP_DecryptFinal_ex(ctx,
                          reinterpret_cast<unsigned char*>(output.data() + outLen1), &outLen2);
    EVP_CIPHER_CTX_free(ctx);

    output.resize(outLen1 + outLen2);
    return output;
}

QByteArray CryptoService::decryptAES(const QByteArray& ciphertextBase64,
                                      const QByteArray& key,
                                      const QByteArray& iv) {
    QByteArray data = base64Decode(ciphertextBase64);
    if (data.isEmpty()) return QByteArray();
    QByteArray cipher = data;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    QByteArray output(cipher.size() + 32, 0);
    int outLen1 = 0, outLen2 = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                        reinterpret_cast<const unsigned char*>(key.constData()),
                        reinterpret_cast<const unsigned char*>(iv.constData()));
    EVP_DecryptUpdate(ctx,
                        reinterpret_cast<unsigned char*>(output.data()), &outLen1,
                        reinterpret_cast<const unsigned char*>(cipher.constData()),
                        cipher.size());
    EVP_DecryptFinal_ex(ctx,
                          reinterpret_cast<unsigned char*>(output.data() + outLen1), &outLen2);
    EVP_CIPHER_CTX_free(ctx);

    output.resize(outLen1 + outLen2);
    return output;
}

QMap<QString, QString> CryptoService::generateRSAKeyPair() {
    QMap<QString, QString> result;
    int bits = 2048;
    unsigned long e = RSA_F4; // 65537

    RSA* rsa = RSA_new();
    BIGNUM* bn = BN_new();
    BN_set_word(bn, e);

    if (!RSA_generate_key_ex(rsa, bits, bn, nullptr)) {
        char err[256];
        ERR_error_string_n(ERR_get_error(), err, sizeof(err));
        qWarning() << "[Crypto] RSA密钥生成失败:" << err;
        BN_free(bn);
        RSA_free(rsa);
        return result;
    }

    BN_free(bn);

    // 公钥 PEM
    BIO* pubBio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPublicKey(pubBio, rsa);
    char* pubData = nullptr;
    long pubLen = BIO_get_mem_data(pubBio, &pubData);
    result["publicKey"] = QByteArray(pubData, pubLen);
    BIO_free(pubBio);

    // 私钥 PEM
    BIO* privBio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPrivateKey(privBio, rsa, nullptr, nullptr, 0, nullptr, nullptr);
    char* privData = nullptr;
    long privLen = BIO_get_mem_data(privBio, &privData);
    result["privateKey"] = QByteArray(privData, privLen);
    BIO_free(privBio);

    RSA_free(rsa);
    qDebug() << "[Crypto] RSA-2048 密钥对生成成功";
    return result;
}

QByteArray CryptoService::rsaEncrypt(const QByteArray& plaintext,
                                      const QString& publicKeyPEM) {
    BIO* bio = BIO_new_mem_buf(publicKeyPEM.constData(), publicKeyPEM.size());
    RSA* rsa = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!rsa) return QByteArray();

    QByteArray output(RSA_size(rsa), 0);
    int len = RSA_public_encrypt(plaintext.size(),
                                  reinterpret_cast<const unsigned char*>(plaintext.constData()),
                                  reinterpret_cast<unsigned char*>(output.data()),
                                  rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    if (len < 0) return QByteArray();
    output.resize(len);
    return base64Encode(output);
}

QByteArray CryptoService::rsaDecrypt(const QByteArray& ciphertextBase64,
                                      const QString& privateKeyPEM) {
    QByteArray cipher = base64Decode(ciphertextBase64);
    BIO* bio = BIO_new_mem_buf(privateKeyPEM.constData(), privateKeyPEM.size());
    RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!rsa) return QByteArray();

    QByteArray output(RSA_size(rsa), 0);
    int len = RSA_private_decrypt(cipher.size(),
                                    reinterpret_cast<const unsigned char*>(cipher.constData()),
                                    reinterpret_cast<unsigned char*>(output.data()),
                                    rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    if (len < 0) return QByteArray();
    output.resize(len);
    return output;
}

QByteArray CryptoService::sha256(const QByteArray& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.constData()),
           data.size(), hash);
    return QByteArray(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

QByteArray CryptoService::signData(const QByteArray& data,
                                    const QString& privateKeyPEM) {
    QByteArray hash = sha256(data);

    BIO* bio = BIO_new_mem_buf(privateKeyPEM.constData(), privateKeyPEM.size());
    RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!rsa) return QByteArray();

    QByteArray signature(RSA_size(rsa), 0);
    unsigned int sigLen = 0;
    int ok = RSA_sign(NID_sha256,
                      reinterpret_cast<const unsigned char*>(hash.constData()),
                      SHA256_DIGEST_LENGTH,
                      reinterpret_cast<unsigned char*>(signature.data()),
                      &sigLen, rsa);
    RSA_free(rsa);
    if (!ok) return QByteArray();
    signature.resize(sigLen);
    return base64Encode(signature);
}

bool CryptoService::verifySignature(const QByteArray& data,
                                    const QByteArray& signatureBase64,
                                    const QString& publicKeyPEM) {
    QByteArray hash = sha256(data);
    QByteArray sig = base64Decode(signatureBase64);

    BIO* bio = BIO_new_mem_buf(publicKeyPEM.constData(), publicKeyPEM.size());
    RSA* rsa = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!rsa) return false;

    int ok = RSA_verify(NID_sha256,
                         reinterpret_cast<const unsigned char*>(hash.constData()),
                         SHA256_DIGEST_LENGTH,
                         reinterpret_cast<const unsigned char*>(sig.constData()),
                         sig.size(), rsa);
    RSA_free(rsa);
    return ok == 1;
}

bool CryptoService::encryptFile(const QString& plaintextPath,
                                const QString& ciphertextPath,
                                const QByteArray& key) {
    QFile inFile(plaintextPath);
    if (!inFile.open(QFile::ReadOnly)) return false;
    QByteArray plain = inFile.readAll();
    inFile.close();

    QByteArray iv;
    generateAESKey(const_cast<QByteArray&>(key), iv); // 实际应该重新生成IV
    // 重新生成
    QByteArray iv2;
    iv2.resize(16);
    RAND_bytes(reinterpret_cast<unsigned char*>(iv2.data()), 16);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    QByteArray cipher(plain.size() + 64, 0);
    int outLen1 = 0, outLen2 = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                        reinterpret_cast<const unsigned char*>(key.constData()),
                        reinterpret_cast<const unsigned char*>(iv2.constData()));
    EVP_EncryptUpdate(ctx,
                        reinterpret_cast<unsigned char*>(cipher.data()), &outLen1,
                        reinterpret_cast<const unsigned char*>(plain.constData()),
                        plain.size());
    EVP_EncryptFinal_ex(ctx,
                          reinterpret_cast<unsigned char*>(cipher.data() + outLen1), &outLen2);
    EVP_CIPHER_CTX_free(ctx);
    cipher.resize(outLen1 + outLen2);

    QFile outFile(ciphertextPath);
    if (!outFile.open(QFile::WriteOnly)) return false;
    outFile.write(iv2);
    outFile.write(cipher);
    outFile.close();
    return true;
}

bool CryptoService::decryptFile(const QString& ciphertextPath,
                                const QString& plaintextPath,
                                const QByteArray& key) {
    QFile inFile(ciphertextPath);
    if (!inFile.open(QFile::ReadOnly)) return false;
    QByteArray iv = inFile.read(16);
    QByteArray cipher = inFile.readAll();
    inFile.close();
    if (iv.size() < 16) return false;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    QByteArray plain(cipher.size() + 64, 0);
    int outLen1 = 0, outLen2 = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                        reinterpret_cast<const unsigned char*>(key.constData()),
                        reinterpret_cast<const unsigned char*>(iv.constData()));
    EVP_DecryptUpdate(ctx,
                        reinterpret_cast<unsigned char*>(plain.data()), &outLen1,
                        reinterpret_cast<const unsigned char*>(cipher.constData()),
                        cipher.size());
    EVP_DecryptFinal_ex(ctx,
                          reinterpret_cast<unsigned char*>(plain.data() + outLen1), &outLen2);
    EVP_CIPHER_CTX_free(ctx);
    plain.resize(outLen1 + outLen2);

    QFile outFile(plaintextPath);
    if (!outFile.open(QFile::WriteOnly)) return false;
    outFile.write(plain);
    outFile.close();
    return true;
}

QByteArray CryptoService::base64Encode(const QByteArray& data) {
    return ::base64Encode(data);
}

QByteArray CryptoService::base64Decode(const QByteArray& data) {
    return ::base64Decode(data);
}
