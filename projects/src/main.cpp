// PrisonRecordTool - Entry Point
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include "ui/MainWindow.h"
#include "ui/LoginDialog.h"
#include "ui/PINDialog.h"
#include "database/DatabaseManager.h"
#include "services/AuditService.h"
#include "services/P2PNodeService.h"
#include "services/KeyManager.h"
#include "services/AuthService.h"
#include "services/CryptoService.h"
#include "utils/PlatformUtils.h"

int main(int argc, char *argv[])
{
    // 跨平台初始化
    PlatformUtils::initRandom();
    PlatformUtils::setDPIAware();

    QApplication app(argc, argv);
    app.setApplicationName("PrisonRecordTool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PrisonSystem");
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    dbPath += "/prison_record.db";

    if (!DatabaseManager::instance().initDatabase(dbPath)) {
        QMessageBox::critical(nullptr, "数据库错误",
            "无法初始化数据库！\n路径: " + dbPath);
        return 1;
    }

    AuditService::instance().logSystemEvent("APP_START", "System",
        "Application started. DB: " + dbPath);

    LoginDialog loginDlg;
    if (loginDlg.exec() != QDialog::Accepted) return 0;

    // 登录成功：PIN 验证（首次设置或解锁数据库主密钥）
    if (KeyManager::instance().needsSetup()) {
        PINDialog pinDlg(nullptr);
        if (pinDlg.exec() != QDialog::Accepted) return 0;
    } else {
        PINDialog pinDlg(true, nullptr);
        if (pinDlg.exec() != QDialog::Accepted) return 0;
    }

    // PIN 解锁成功：启动 P2P 去中心化节点服务
    // 从本地存储的公钥启动节点，节点 ID = SHA-256(公钥)
    QString uid = AuthService::instance().currentUser().userId;
    QString keysDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/keys/" + uid;
    QString pubKeyPath = keysDir + "/public_key.pem";
    QString publicKeyPEM;
    QFile pkFile(pubKeyPath);
    if (pkFile.open(QIODevice::ReadOnly)) {
        publicKeyPEM = QString::fromUtf8(pkFile.readAll());
        pkFile.close();
    }
    if (!publicKeyPEM.isEmpty()) {
        P2PNodeService::instance().start(uid, publicKeyPEM);
        qDebug() << "[主程序] P2P 节点服务已启动，节点 ID:"
                 << QString::fromUtf8(
                        CryptoService::instance().sha256(publicKeyPEM.toUtf8()).toHex().left(16));
    } else {
        qDebug() << "[主程序] 未找到公钥文件，跳过 P2P 启动（用户尚未生成密钥）";
    }

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}

