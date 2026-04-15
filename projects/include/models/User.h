#ifndef USER_H
#define USER_H
#include <QString>
#include <QDateTime>
enum class UserRole { Admin, Legal, Interrogator, ReadOnly };
struct User {
    int id = 0;
    QString userId, username, passwordHash, realName;
    UserRole role = UserRole::ReadOnly;
    QString department, position, phone;
    bool enabled = true;
    int failedLoginAttempts = 0;
    QDateTime lockedUntil, lastLoginTime, createdAt;
    bool isLocked() const;
    bool canDelete() const;
    bool canEdit() const;         // ReadOnly 用户不能新建/编辑/删除
    bool canApprove() const;     // 只有 Legal 用户能审核
    QString roleString() const;
};
#endif
