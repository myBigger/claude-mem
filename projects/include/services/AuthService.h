#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H
#include <QString>
#include "models/User.h"
class AuthService {
public:
    static AuthService& instance();
    bool login(const QString& username, const QString& password);
    void logout();
    User currentUser() const { return m_currentUser; }
    bool isLoggedIn() const { return m_currentUser.id > 0; }
    bool isAdmin() const { return m_currentUser.role == UserRole::Admin; }
    bool canEdit() const { return m_currentUser.canEdit(); }
    bool canApprove() const { return m_currentUser.canApprove(); }
    bool changePassword(int userId, const QString& oldPwd, const QString& newPwd);
    void resetPassword(int userId);
    bool isAccountLocked(const QString& username);
private:
    AuthService(); ~AuthService(); AuthService(const AuthService&) = delete;
    User m_currentUser;
    static const QString SALT;
};
#endif
