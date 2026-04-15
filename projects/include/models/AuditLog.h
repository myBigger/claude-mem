#ifndef AUDITLOG_H
#define AUDITLOG_H
#include <QString>
struct AuditLog {
    int id=0, userId=0, targetId=0;
    QString username, action, module, targetType, details, ipAddress, timestamp;
};
#endif
