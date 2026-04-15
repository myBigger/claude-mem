#include "ui/widgets/BackupWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "utils/PlatformUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QGroupBox>

BackupWidget::BackupWidget(QWidget* p) : QWidget(p) {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(12);

    QGroupBox* infoBox = new QGroupBox("数据库信息");
    QFormLayout* infoForm = new QFormLayout;
    m_dbPathLabel = new QLabel(DatabaseManager::instance().databasePath());
    m_dbPathLabel->setStyleSheet("color:#E6EDF3;font-weight:bold;word-break:break-all;font-size:12px;");
    QFileInfo fi(DatabaseManager::instance().databasePath());
    m_dbSizeLabel = new QLabel(fi.exists() ? QString("%1 KB").arg(fi.size()/1024) : "--");
    m_lastBackupLabel = new QLabel("--");
    infoForm->addRow("数据库路径：", m_dbPathLabel);
    infoForm->addRow("当前大小：", m_dbSizeLabel);
    infoForm->addRow("最近备份：", m_lastBackupLabel);
    infoBox->setLayout(infoForm);
    main->addWidget(infoBox);

    QGroupBox* opBox = new QGroupBox("备份操作");
    QHBoxLayout* opBar = new QHBoxLayout;
    QPushButton* backupBtn = new QPushButton("立即备份");
    QPushButton* restoreBtn = new QPushButton("还原数据");
    QPushButton* exportBtn = new QPushButton("导出备份");
    backupBtn->setStyleSheet("QPushButton{background:#0066CC;color:#FFFFFF;padding:8px 20px;border:none;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#0052A3;}");
    restoreBtn->setStyleSheet("QPushButton{background:#E53E3E;color:#FFFFFF;padding:8px 20px;border:none;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#C53030;}");
    exportBtn->setStyleSheet("QPushButton{background:#22C55E;color:#FFFFFF;padding:8px 20px;border:none;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#16A34A;}");
    opBar->addWidget(backupBtn);
    opBar->addWidget(restoreBtn);
    opBar->addWidget(exportBtn);
    opBar->addStretch();
    opBox->setLayout(opBar);
    main->addWidget(opBox);

    QGroupBox* histBox = new QGroupBox("备份历史");
    QVBoxLayout* histLayout = new QVBoxLayout(histBox);
    m_historyTable = new QTableWidget;
    m_historyTable->setColumnCount(4);
    m_historyTable->setHorizontalHeaderLabels({"文件名","备份时间","大小","操作"});
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setAlternatingRowColors(true);
    histLayout->addWidget(m_historyTable);
    main->addWidget(histBox, 1);

    connect(backupBtn, &QPushButton::clicked, this, &BackupWidget::onBackup);
    connect(restoreBtn, &QPushButton::clicked, this, &BackupWidget::onRestore);
    connect(exportBtn, &QPushButton::clicked, this, &BackupWidget::onExportBackup);
    connect(m_historyTable, &QTableWidget::cellDoubleClicked, this, &BackupWidget::onRestoreHistory);

    loadHistory();
}

void BackupWidget::loadHistory() {
    // 使用跨平台备份目录
    QString backupDirPath = PlatformUtils::getBackupDir();
    QDir backupDir(backupDirPath);
    if (!backupDir.exists()) backupDir.mkpath(".");
    QFileInfoList files = backupDir.entryInfoList({"*.db"}, QDir::Files, QDir::Time);
    m_historyTable->setRowCount(files.size());
    for (int i = 0; i < files.size(); ++i) {
        QFileInfo f = files[i];
        m_historyTable->setItem(i, 0, new QTableWidgetItem(f.fileName()));
        m_historyTable->setItem(i, 1, new QTableWidgetItem(f.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
        m_historyTable->setItem(i, 2, new QTableWidgetItem(QString("%1 KB").arg(f.size()/1024)));
        m_historyTable->setItem(i, 3, new QTableWidgetItem("双击还原"));
        m_historyTable->item(i,3)->setForeground(QBrush(QColor("#E53E3E")));
        m_historyTable->item(i,0)->setData(Qt::UserRole, f.absoluteFilePath());
    }
    if (!files.isEmpty()) {
        QFileInfo f = files.first();
        m_lastBackupLabel->setText(f.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
    }
}

void BackupWidget::onBackup() {
    QString defaultName = "prison_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".db";
    // 使用跨平台备份目录
    QString saveDir = PlatformUtils::getBackupDir();
    QDir().mkpath(saveDir);
    QString path = QFileDialog::getSaveFileName(this, "保存备份",
        saveDir + "/" + defaultName, "SQLite(*.db)");
    if (path.isEmpty()) return;
    if (DatabaseManager::instance().backup(path)) {
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_CREATE, "数据备份", "Database", 0, "备份:" + path);
        QMessageBox::information(this, "成功", "数据库已备份到：\n" + path);
        loadHistory();
        QFileInfo fi(path);
        m_lastBackupLabel->setText(fi.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
    } else {
        QMessageBox::warning(this, "失败", "备份失败，请检查磁盘空间或权限");
    }
}

void BackupWidget::onRestore() {
    if (QMessageBox::warning(this, "确认还原",
        "还原将覆盖当前所有数据！\n建议先做好备份再操作。\n\n是否继续？",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    // 使用跨平台备份目录
    QString path = QFileDialog::getOpenFileName(this, "选择备份文件",
        PlatformUtils::getBackupDir(), "SQLite(*.db)");
    if (path.isEmpty()) return;
    if (QMessageBox::warning(this, "二次确认",
        "最后一次确认！\n将从以下文件还原：\n" + path + "\n\n所有当前数据将被覆盖！",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    if (DatabaseManager::instance().restore(path)) {
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_UPDATE, "数据还原", "Database", 0, "还原:" + path);
        QMessageBox::information(this, "成功", "数据已还原，请重启程序使更改生效");
    } else {
        QMessageBox::warning(this, "失败", "还原失败，可能文件已损坏");
    }
}

void BackupWidget::onRestoreHistory(int row, int) {
    if (row < 0) return;
    QString path = m_historyTable->item(row, 0)->data(Qt::UserRole).toString();
    QString fileName = m_historyTable->item(row, 0)->text();
    if (QMessageBox::warning(this, "确认还原",
        QString("确定要从以下备份还原？\n%1\n\n当前数据将被覆盖！").arg(fileName),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    if (DatabaseManager::instance().restore(path)) {
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_UPDATE, "数据还原", "Database", 0, "历史还原:" + fileName);
        QMessageBox::information(this, "成功", "数据已还原，请重启程序使更改生效");
    } else {
        QMessageBox::warning(this, "失败", "还原失败");
    }
}

void BackupWidget::onExportBackup() {
    // 使用跨平台文档目录作为默认导出位置
    QString dir = QFileDialog::getExistingDirectory(this, "选择导出目录",
        PlatformUtils::getDocumentsDir());
    if (dir.isEmpty()) return;
    QString defaultName = "prison_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".db";
    QString srcPath = DatabaseManager::instance().databasePath();
    QString destPath = dir + "/" + defaultName;
    if (QFile::copy(srcPath, destPath)) {
        QMessageBox::information(this, "成功", "备份已导出到：\n" + destPath);
    } else {
        QMessageBox::warning(this, "失败", "导出失败，请检查目录权限");
    }
}
