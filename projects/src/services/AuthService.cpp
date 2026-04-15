#include "services/AuthService.h"
#include "database/DatabaseManager.h"
#include "services/AuditService.h"
#include "utils/PasswordHasher.h"
#include <QDebug>
#include <QDateTime>

const QString AuthService::SALT = "prison_salt_2024";

AuthService::AuthService() {}
AuthService::~AuthService() {}
AuthService& AuthService::instance() { static AuthService i; return i; }

bool AuthService::login(const QString& username, const QString& password) {
    if (isAccountLocked(username)) { qWarning() << "[认证] 账户已锁定:" << username; return false; }
    DatabaseManager& db = DatabaseManager::instance();
    auto result = db.selectOne("SELECT * FROM users WHERE username=:u AND enabled=1", {{"u",username}});
    if (result.isEmpty()) { qWarning() << "[认证] 用户不存在:" << username; return false; }
    QString storedHash = result["password_hash"].toString();
    QString inputHash = PasswordHasher::hash(password, SALT);
    if (storedHash != inputHash) {
        int attempts = result["failed_login_attempts"].toInt() + 1;
        QMap<QString,QVariant> vals = {{"failed_login_attempts", attempts}};
        if (attempts >= 5) vals["locked_until"] = QDateTime::currentDateTime().addSecs(1800).toString("yyyy-MM-dd hh:mm:ss");
        db.update("users", vals, "username=:u", {{"u",username}});
        return false;
    }
    db.update("users", {{"last_login_time", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")},
        {"failed_login_attempts", 0}}, "username=:u", {{"u",username}});
    m_currentUser.id = result["id"].toInt();
    m_currentUser.userId = result["user_id"].toString();
    m_currentUser.username = result["username"].toString();
    m_currentUser.realName = result["real_name"].toString();
    m_currentUser.role = result["role"].toString() == "Admin" ? UserRole::Admin
                       : result["role"].toString() == "Legal" ? UserRole::Legal
                       : result["role"].toString() == "Interrogator" ? UserRole::Interrogator
                       : UserRole::ReadOnly;
    m_currentUser.department = result["department"].toString();
    m_currentUser.position = result["position"].toString();
    AuditService::instance().setCurrentUser(m_currentUser.id, m_currentUser.realName, m_currentUser.roleString());
    AuditService::instance().log(m_currentUser.id, m_currentUser.realName, AuditService::ACTION_LOGIN, "认证", "User", m_currentUser.id, "登录成功");
    qDebug() << "[认证] 登录成功:" << username;
    return true;
}

void AuthService::logout() {
    if (m_currentUser.id > 0)
        AuditService::instance().log(m_currentUser.id, m_currentUser.realName, AuditService::ACTION_LOGOUT, "认证", "User", m_currentUser.id, "登出");
    m_currentUser = User();
}

bool AuthService::isAccountLocked(const QString& username) {
    auto r = DatabaseManager::instance().selectOne("SELECT locked_until FROM users WHERE username=:u", {{"u",username}});
    if (r.isEmpty()) return false;
    QString locked = r["locked_until"].toString();
    if (locked.isEmpty()) return false;
    return QDateTime::currentDateTime() < QDateTime::fromString(locked, "yyyy-MM-dd hh:mm:ss");
}

bool AuthService::changePassword(int userId, const QString& oldPwd, const QString& newPwd) {
    auto r = DatabaseManager::instance().selectOne("SELECT password_hash FROM users WHERE id=:id", {{"id",userId}});
    if (r.isEmpty() || r["password_hash"].toString() != PasswordHasher::hash(oldPwd, SALT)) return false;
    return DatabaseManager::instance().update("users", {{"password_hash", PasswordHasher::hash(newPwd, SALT)}},
        "id=:id", {{"id", userId}});
}

void AuthService::resetPassword(int userId) {
    DatabaseManager::instance().update("users", {{"password_hash","098f6bcd4621d373cade4e832627b4f6"},{"failed_login_attempts",0}},
        "id=:id", {{"id",userId}});
}
