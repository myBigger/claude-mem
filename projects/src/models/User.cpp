#include "models/User.h"
bool User::isLocked() const { return lockedUntil.isValid() && QDateTime::currentDateTime() < lockedUntil; }
bool User::canDelete() const { return role == UserRole::Admin; }
bool User::canEdit() const { return role != UserRole::ReadOnly; }
bool User::canApprove() const { return role == UserRole::Admin || role == UserRole::Legal; }
QString User::roleString() const {
    switch (role) {
        case UserRole::Admin:       return "系统管理员";
        case UserRole::Legal:       return "法制科";
        case UserRole::Interrogator: return "审讯员";
        case UserRole::ReadOnly:    return "信息科（只读）";
        default:                    return "未知";
    }
}
