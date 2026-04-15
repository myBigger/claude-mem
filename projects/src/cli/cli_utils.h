#ifndef CLI_UTILS_H
#define CLI_UTILS_H
#include <QString>

class CliUtils {
public:
    // 查找数据库路径（跨平台）
    static QString findDatabasePath();

    // 初始化数据库
    static bool initDatabase(const QString& dbPath);

    // 解锁加密内容（需要 PIN）
    static bool unlockEncryption(const QString& pin);

    // 设置 CLI 模拟用户（用于审计日志）
    static void setCurrentUser(int userId, const QString& username, const QString& role);

    // 解密字段值（如果已加密）
    static QString decryptField(const QString& value);
};

#endif // CLI_UTILS_H
