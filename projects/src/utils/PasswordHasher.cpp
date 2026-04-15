#include "utils/PasswordHasher.h"
#include <QCryptographicHash>
QString PasswordHasher::md5(const QString& input) {
    return QString(QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5).toHex());
}
QString PasswordHasher::hash(const QString& password, const QString& salt) {
    return md5(password + "_" + salt);
}
bool PasswordHasher::verify(const QString& password, const QString& storedHash, const QString& salt) {
    return hash(password, salt) == storedHash;
}
