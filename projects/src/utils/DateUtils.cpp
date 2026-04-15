#include "utils/DateUtils.h"
#include <QDateTime>
#include <QDate>
QString DateUtils::currentDateTime() { return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"); }
QString DateUtils::currentDate() { return QDate::currentDate().toString("yyyy-MM-dd"); }
QString DateUtils::currentDateForId() { return QDate::currentDate().toString("yyyyMMdd"); }
QString DateUtils::generateId(const QString& prefix) {
    static int counter = 1;
    return QString("%1-%2-%3").arg(prefix).arg(currentDateForId()).arg(counter++, 4, 10, QChar('0'));
}
QString DateUtils::maskIdCard(const QString& id) {
    if (id.length() < 10) return id;
    return id.left(6) + "***" + id.right(4);
}
QString DateUtils::maskPhone(const QString& phone) {
    if (phone.length() < 7) return phone;
    return phone.left(3) + "****" + phone.right(4);
}
