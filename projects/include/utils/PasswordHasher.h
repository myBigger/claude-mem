#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H
#include <QString>
class PasswordHasher {
public:
    static QString hash(const QString& password, const QString& salt);
    static bool verify(const QString& password, const QString& hash, const QString& salt);
    static QString md5(const QString& input);
};
#endif
