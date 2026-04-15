#include "services/AuditService.h"
#include "database/DatabaseManager.h"
#include <QDateTime>
#include <QDebug>

const QString AuditService::ACTION_LOGIN="登录";
const QString AuditService::ACTION_LOGOUT="登出";
const QString AuditService::ACTION_CREATE="新增";
const QString AuditService::ACTION_UPDATE="编辑";
const QString AuditService::ACTION_DELETE="删除";
const QString AuditService::ACTION_VIEW="查看";
const QString AuditService::ACTION_APPROVE="审批通过";
const QString AuditService::ACTION_REJECT="审批驳回";
const QString AuditService::ACTION_EXPORT="数据导出";
const QString AuditService::ACTION_PRINT="打印";
const QString AuditService::ACTION_SUBMIT_APPROVAL="提交审批";

AuditService::AuditService() {}
AuditService::~AuditService() {}
AuditService& AuditService::instance() { static AuditService i; return i; }

void AuditService::setCurrentUser(int id, const QString& name, const QString& role) {
    m_currentUserId = id; m_currentUserName = name; m_currentUserRole = role;
}

void AuditService::log(int userId, const QString& username, const QString& action,
                         const QString& module, const QString& targetType,
                         int targetId, const QString& details) {
    DatabaseManager::instance().insert("audit_logs", {
        {"user_id", userId}, {"username", username}, {"action", action},
        {"module", module}, {"target_type", targetType}, {"target_id", targetId},
        {"details", details}, {"timestamp", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")},
        {"ip_address", "127.0.0.1"}
    });
}

void AuditService::logSystemEvent(const QString& action, const QString& module, const QString& details) {
    log(m_currentUserId, m_currentUserName.isEmpty() ? "System" : m_currentUserName,
        action, module, "System", 0, details);
}

QVector<QMap<QString,QVariant>> AuditService::queryLogs(const QString& startDate,
    const QString& endDate, const QString& username, const QString& action,
    int limit, int offset) {
    QStringList cond; QMap<QString,QVariant> p;
    if (!startDate.isEmpty()) { cond << "timestamp >= :sd"; p["sd"] = startDate + " 00:00:00"; }
    if (!endDate.isEmpty()) { cond << "timestamp <= :ed"; p["ed"] = endDate + " 23:59:59"; }
    if (!username.isEmpty()) { cond << "username LIKE :un"; p["un"] = "%" + username + "%"; }
    if (!action.isEmpty()) { cond << "action = :ac"; p["ac"] = action; }
    QString where = cond.isEmpty() ? "" : "WHERE " + cond.join(" AND ");
    QString sql = QString("SELECT * FROM audit_logs %1 ORDER BY timestamp DESC LIMIT :l OFFSET :o").arg(where);
    p["l"] = limit; p["o"] = offset;
    return DatabaseManager::instance().select(sql, p);
}

int AuditService::queryLogCount(const QString& startDate, const QString& endDate,
                                  const QString& username, const QString& action) {
    QStringList cond; QMap<QString,QVariant> p;
    if (!startDate.isEmpty()) { cond << "timestamp >= :sd"; p["sd"] = startDate + " 00:00:00"; }
    if (!endDate.isEmpty()) { cond << "timestamp <= :ed"; p["ed"] = endDate + " 23:59:59"; }
    if (!username.isEmpty()) { cond << "username LIKE :un"; p["un"] = "%" + username + "%"; }
    if (!action.isEmpty()) { cond << "action = :ac"; p["ac"] = action; }
    QString where = cond.isEmpty() ? "" : "WHERE " + cond.join(" AND ");
    auto r = DatabaseManager::instance().selectOne("SELECT COUNT(*) as cnt FROM audit_logs " + where, p);
    return r.value("cnt").toInt();
}
