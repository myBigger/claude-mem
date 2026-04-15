#ifndef DATEUTILS_H
#define DATEUTILS_H
#include <QString>
class DateUtils {
public:
    static QString currentDateTime();
    static QString currentDate();
    static QString currentDateForId();
    static QString generateId(const QString& prefix);
    static QString maskIdCard(const QString& id);
    static QString maskPhone(const QString& phone);
};
#endif
