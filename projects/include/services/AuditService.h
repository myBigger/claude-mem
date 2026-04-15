#ifndef AUDITSERVICE_H
#define AUDITSERVICE_H
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
class AuditService {
public:
    static AuditService& instance();
    void log(int userId, const QString& username, const QString& action,
              const QString& module, const QString& targetType="", int targetId=0,
              const QString& details="");
    void logSystemEvent(const QString& action, const QString& module, const QString& details="");
    QVector<QMap<QString,QVariant>> queryLogs(const QString& startDate="",
        const QString& endDate="", const QString& username="", const QString& action="",
        int limit=100, int offset=0);
    int queryLogCount(const QString& startDate="", const QString& endDate="",
                       const QString& username="", const QString& action="");
    int currentUserId() const { return m_currentUserId; }
    QString currentUserName() const { return m_currentUserName; }
    QString currentUserRole() const { return m_currentUserRole; }
    void setCurrentUser(int userId, const QString& username, const QString& role);
    static const QString ACTION_LOGIN, ACTION_LOGOUT, ACTION_CREATE, ACTION_UPDATE,
           ACTION_DELETE, ACTION_VIEW, ACTION_APPROVE, ACTION_REJECT, ACTION_EXPORT,
           ACTION_PRINT, ACTION_SUBMIT_APPROVAL;
private:
    AuditService(); ~AuditService(); AuditService(const AuditService&) = delete;
    int m_currentUserId = 0;
    QString m_currentUserName, m_currentUserRole;
};
#endif
