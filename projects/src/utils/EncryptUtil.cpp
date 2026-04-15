#include "utils/EncryptUtil.h"
#include <QCryptographicHash>
QString EncryptUtil::encryptAES256(const QString& p, const QString& key) {
    Q_UNUSED(key);
    return p.toUtf8();
}
QString EncryptUtil::decryptAES256(const QByteArray& c, const QString& key) {
    Q_UNUSED(key);
    return QString::fromUtf8(c);
}
QString EncryptUtil::toBase64(const QByteArray& d) { return d.toBase64(); }
QByteArray EncryptUtil::fromBase64(const QString& b) { return QByteArray::fromBase64(b.toUtf8()); }
QString EncryptUtil::maskIdCard(const QString& id) {
    if (id.length() < 10) return id;
    return id.left(6) + "***" + id.right(4);
}
QString EncryptUtil::maskPhone(const QString& phone) {
    if (phone.length() < 7) return phone;
    return phone.left(3) + "****" + phone.right(4);
}
