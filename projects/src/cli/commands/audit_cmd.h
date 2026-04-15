#ifndef AUDIT_CMD_H
#define AUDIT_CMD_H
#include <QString>
class AuditCmd {
public:
    static int run(const QStringList& args);
};

#endif // AUDIT_CMD_H
