#include "cli_utils.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <iostream>
#include <database/DatabaseManager.h>
#include <services/KeyManager.h>
#include <services/AuditService.h>

QString CliUtils::findDatabasePath() {
    // 查找顺序：1. 标准数据目录  2. 当前目录  3. 构建目录
    QStringList searchPaths;

    // 1. 标准数据目录（跨平台）
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataDir.isEmpty()) {
        searchPaths << dataDir;
    }

    // 2. 当前工作目录
    searchPaths << QDir::currentPath();

    // 3. 构建目录（开发时用）
#ifdef _WIN32
    searchPaths << QCoreApplication::applicationDirPath() + "/../../../data";
#else
    searchPaths << QCoreApplication::applicationDirPath() + "/../../data";
#endif

    // 4. ~/.prison-sis
    QString homeDb = QDir::homePath() + "/.prison-sis";
    searchPaths << homeDb;

    const QString dbName = "prison.db";
    for (const QString& dir : searchPaths) {
        QDir d(dir);
        if (d.exists(dbName)) {
            return d.filePath(dbName);
        }
        // 也尝试 .db 文件
        QStringList entries = d.entryList(QStringList() << "*.db", QDir::Files);
        if (!entries.isEmpty()) {
            return d.filePath(entries.first());
        }
    }
    return QString();
}

bool CliUtils::initDatabase(const QString& dbPath) {
    if (!DatabaseManager::instance().initDatabase(dbPath)) {
        std::cerr << "错误: 无法打开数据库: " << qPrintable(dbPath) << "\n";
        return false;
    }
    if (!DatabaseManager::instance().initSchema()) {
        std::cerr << "错误: 无法初始化数据库架构。\n";
        return false;
    }
    return true;
}

bool CliUtils::unlockEncryption(const QString& pin) {
    KeyManager& km = KeyManager::instance();
    if (km.needsSetup()) {
        return false;
    }
    return km.unlock(pin);
}

void CliUtils::setCurrentUser(int userId, const QString& username, const QString& role) {
    AuditService::instance().setCurrentUser(userId, username, role);
}

QString CliUtils::decryptField(const QString& value) {
    return KeyManager::decryptContent(value);
}
