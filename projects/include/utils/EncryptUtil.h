#ifndef ENCRYPTUTIL_H
#define ENCRYPTUTIL_H
#include <QString>
#include <QByteArray>
class EncryptUtil {
public:
    static QString encryptAES256(const QString& plaintext, const QString& key);
    static QString decryptAES256(const QByteArray& ciphertext, const QString& key);
    static QString toBase64(const QByteArray& data);
    static QByteArray fromBase64(const QString& base64);
    static QString maskIdCard(const QString& id);
    static QString maskPhone(const QString& phone);
};
#endif
