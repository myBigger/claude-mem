#include "ui/widgets/ApprovalWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "services/KeyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QDateTime>
#include <QInputDialog>

ApprovalWidget::ApprovalWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadApprovals();
}

void ApprovalWidget::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(6);

    // 标签页
    m_tabs = new QTabWidget;
    m_pendingTable = new QTableWidget;
    m_inApprovalTable = new QTableWidget;
    m_approvedTable = new QTableWidget;
    m_rejectedTable = new QTableWidget;

    auto setupTable = [](QTableWidget* t) {
        t->setColumnCount(6);
        t->setHorizontalHeaderLabels({"笔录编号","罪犯姓名","笔录类型","提交人","提交时间","操作"});
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setAlternatingRowColors(true);
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        t->verticalHeader()->setVisible(false);
    };
    setupTable(m_pendingTable);
    setupTable(m_inApprovalTable);
    setupTable(m_approvedTable);
    setupTable(m_rejectedTable);

    m_tabs->addTab(m_pendingTable, "🔴 待我审批");
    m_tabs->addTab(m_inApprovalTable, "⏳ 审批中");
    m_tabs->addTab(m_approvedTable, "✅ 已通过");
    m_tabs->addTab(m_rejectedTable, "❌ 已驳回");

    // 预览区
    QGroupBox* previewBox = new QGroupBox("笔录内容预览（只读）");
    m_preview = new QTextEdit;
    m_preview->setReadOnly(true);
    m_preview->setPlaceholderText("点击左侧表格中的笔录，可在此预览内容");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewBox);
    previewLayout->addWidget(m_preview);

    // 操作按钮
    QHBoxLayout* btnBar = new QHBoxLayout;
    QPushButton* approveBtn = new QPushButton("通过");
    approveBtn->setStyleSheet("background:#22C55E;color:#FFFFFF;padding:8px 20px;font-weight:bold;border:none;border-radius:4px;");
    QPushButton* rejectBtn = new QPushButton("驳回");
    rejectBtn->setStyleSheet("background:#E53E3E;color:#FFFFFF;padding:8px 20px;border:none;border-radius:4px;font-weight:bold;");
    btnBar->addWidget(approveBtn);
    btnBar->addWidget(rejectBtn);
    btnBar->addStretch();

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->addWidget(m_tabs, 1);
    contentLayout->addWidget(previewBox, 1);

    main->addLayout(contentLayout, 1);
    main->addLayout(btnBar);
    main->setContentsMargins(10,10,10,10);

    // 信号连接
    connect(m_tabs, &QTabWidget::currentChanged, this, &ApprovalWidget::onTabChanged);
    connect(m_pendingTable, &QTableWidget::itemClicked, this, &ApprovalWidget::onPreviewRecord);
    connect(m_inApprovalTable, &QTableWidget::itemClicked, this, &ApprovalWidget::onPreviewRecord);
    connect(m_approvedTable, &QTableWidget::itemClicked, this, &ApprovalWidget::onPreviewRecord);
    connect(m_rejectedTable, &QTableWidget::itemClicked, this, &ApprovalWidget::onPreviewRecord);
    connect(approveBtn, &QPushButton::clicked, this, &ApprovalWidget::onApprove);
    connect(rejectBtn, &QPushButton::clicked, this, &ApprovalWidget::onReject);
}

void ApprovalWidget::onTabChanged(int) { loadApprovals(); }

void ApprovalWidget::loadApprovals() {
    auto& auth = AuthService::instance();
    QString userId = auth.currentUser().userId;
    bool isAdmin = auth.isAdmin();

    // 清空所有表
    m_pendingTable->setRowCount(0);
    m_inApprovalTable->setRowCount(0);
    m_approvedTable->setRowCount(0);
    m_rejectedTable->setRowCount(0);

    // 待我审批（一级审批：status=PendingApproval 且 approver1_id为空 或 二级审批：status=InApproval 且 approver1已通过）
    QString sqlPending = R"(
        SELECT r.id, r.record_id, r.criminal_name, r.record_type, r.record_date,
               u.real_name as submitter, r.created_at, r.status,
               r.approver1_result, r.approver2_result
        FROM records r
        LEFT JOIN users u ON r.created_by = u.id
        WHERE (r.status='PendingApproval' OR r.status='InApproval')
        ORDER BY r.created_at DESC
    )";
    auto pending = DatabaseManager::instance().select(sqlPending, {});

    for (const auto& r : pending) {
        QString status = r["status"].toString();
        QTableWidget* targetTable = nullptr;
        if (status == "PendingApproval") targetTable = m_pendingTable;
        else if (status == "InApproval") targetTable = m_inApprovalTable;
        if (!targetTable) continue;

        int row = targetTable->rowCount();
        targetTable->insertRow(row);
        targetTable->setItem(row, 0, new QTableWidgetItem(r["record_id"].toString()));
        targetTable->setItem(row, 1, new QTableWidgetItem(r["criminal_name"].toString()));
        targetTable->setItem(row, 2, new QTableWidgetItem(r["record_type"].toString()));
        targetTable->setItem(row, 3, new QTableWidgetItem(r["submitter"].toString()));
        targetTable->setItem(row, 4, new QTableWidgetItem(r["created_at"].toString()));
        targetTable->setItem(row, 5, new QTableWidgetItem(
            status == "PendingApproval" ? "待一级审批" : "待二级审批"));
        targetTable->item(row, 0)->setData(Qt::UserRole, r["id"].toInt());
    }

    // 已通过
    auto approved = DatabaseManager::instance().select(
        "SELECT r.id, r.record_id, r.criminal_name, r.record_type, u.real_name, r.created_at "
        "FROM records r LEFT JOIN users u ON r.created_by=u.id "
        "WHERE r.status='Approved' ORDER BY r.updated_at DESC LIMIT 50", {});
    m_approvedTable->setRowCount(approved.size());
    for (int i = 0; i < approved.size(); ++i) {
        m_approvedTable->setItem(i, 0, new QTableWidgetItem(approved[i]["record_id"].toString()));
        m_approvedTable->setItem(i, 1, new QTableWidgetItem(approved[i]["criminal_name"].toString()));
        m_approvedTable->setItem(i, 2, new QTableWidgetItem(approved[i]["record_type"].toString()));
        m_approvedTable->setItem(i, 3, new QTableWidgetItem(approved[i]["real_name"].toString()));
        m_approvedTable->setItem(i, 4, new QTableWidgetItem(approved[i]["created_at"].toString()));
        m_approvedTable->setItem(i, 5, new QTableWidgetItem("已通过"));
    }

    // 已驳回
    auto rejected = DatabaseManager::instance().select(
        "SELECT r.id, r.record_id, r.criminal_name, r.record_type, u.real_name, r.created_at, r.reject_reason "
        "FROM records r LEFT JOIN users u ON r.created_by=u.id "
        "WHERE r.status='Rejected' ORDER BY r.updated_at DESC LIMIT 50", {});
    m_rejectedTable->setRowCount(rejected.size());
    for (int i = 0; i < rejected.size(); ++i) {
        m_rejectedTable->setItem(i, 0, new QTableWidgetItem(rejected[i]["record_id"].toString()));
        m_rejectedTable->setItem(i, 1, new QTableWidgetItem(rejected[i]["criminal_name"].toString()));
        m_rejectedTable->setItem(i, 2, new QTableWidgetItem(rejected[i]["record_type"].toString()));
        m_rejectedTable->setItem(i, 3, new QTableWidgetItem(rejected[i]["real_name"].toString()));
        m_rejectedTable->setItem(i, 4, new QTableWidgetItem(rejected[i]["created_at"].toString()));
        QString reason = rejected[i]["reject_reason"].toString();
        m_rejectedTable->setItem(i, 5, new QTableWidgetItem(
            reason.isEmpty() ? "已驳回" : "驳回:" + reason));
    }
}

void ApprovalWidget::onPreviewRecord(QTableWidgetItem* item) {
    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    if (!table) return;
    int row = item ? item->row() : -1;
    if (row < 0) return;
    int recordId = table->item(row, 0)->data(Qt::UserRole).toInt();
    if (recordId == 0) return;
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM records WHERE id=:id", {{"id", recordId}});
    if (r.isEmpty()) return;
    m_currentRecordId = recordId;
    QString preview = QString(
        "【笔录编号】%1\n"
        "【笔录类型】%2\n"
        "【关联罪犯】%3\n"
        "【审讯时间】%4\n"
        "【审讯地点】%5\n"
        "【讯问人】%6\n"
        "【记录员】%7\n"
        "【在场人员】%8\n"
        "【当前状态】%9\n"
        "━━━━━━━━━━━━━━━━━━━━\n"
        "【笔录正文】\n%10"
    ).arg(r["record_id"].toString())
     .arg(r["record_type"].toString())
     .arg(r["criminal_name"].toString())
     .arg(r["record_date"].toString())
     .arg(r["record_location"].toString())
     .arg(r["interrogator_id"].toString())
     .arg(r["recorder_id"].toString())
     .arg(r["present_persons"].toString())
     .arg(r["status"].toString())
     .arg(KeyManager::decryptContent(r["content"].toString()));
    m_preview->setPlainText(preview);
}

void ApprovalWidget::onApprove() {
    if (m_currentRecordId == 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中一条笔录");
        return;
    }
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM records WHERE id=:id", {{"id", m_currentRecordId}});
    if (r.isEmpty()) return;

    QString status = r["status"].toString();
    auto& auth = AuthService::instance();
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QMap<QString, QVariant> vals;
    QString logMsg;

    if (status == "PendingApproval") {
        // 一级审批通过
        vals = {
            {"status", "InApproval"},
            {"approver1_id", auth.currentUser().id},
            {"approver1_result", "Approved"},
            {"approver1_opinion", ""},
            {"updated_at", now}
        };
        logMsg = "一级审批通过";
    } else if (status == "InApproval") {
        // 二级审批通过 = 最终通过
        vals = {
            {"status", "Approved"},
            {"approver2_id", auth.currentUser().id},
            {"approver2_result", "Approved"},
            {"approver2_opinion", ""},
            {"updated_at", now}
        };
        logMsg = "二级审批通过，笔录归档";
    } else {
        QMessageBox::warning(this, "提示", "该笔录当前状态不允许审批");
        return;
    }

    bool ok = DatabaseManager::instance().update("records", vals,
        "id=:id", {{"id", m_currentRecordId}});
    if (ok) {
        AuditService::instance().log(auth.currentUser().id, auth.currentUser().realName,
            AuditService::ACTION_APPROVE, "审批中心", "Record", m_currentRecordId, logMsg);
        QMessageBox::information(this, "审批成功", logMsg);
        m_currentRecordId = 0;
        m_preview->clear();
        loadApprovals();
    }
}

void ApprovalWidget::onReject() {
    if (m_currentRecordId == 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中一条笔录");
        return;
    }
    bool ok = false;
    QString reason = QInputDialog::getMultiLineText(this, "驳回原因",
        "请填写驳回原因（必填）：", "", &ok);
    if (!ok || reason.trimmed().isEmpty()) return;

    auto& auth = AuthService::instance();
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString opinion = QInputDialog::getText(this, "审批意见",
        "可填写审批意见（选填）：", QLineEdit::Normal, QString(), nullptr);
    if (opinion.isNull()) opinion = "";

    QMap<QString, QVariant> vals = {
        {"status", "Rejected"},
        {"reject_reason", reason.trimmed()},
        {"approver1_opinion", opinion},
        {"updated_at", now}
    };
    // 判断是驳回一级还是二级
    auto r = DatabaseManager::instance().selectOne(
        "SELECT status FROM records WHERE id=:id", {{"id", m_currentRecordId}});
    if (!r.isEmpty() && r["status"].toString() == "InApproval") {
        vals["approver2_result"] = "Rejected";
        vals["approver2_opinion"] = opinion;
    } else {
        vals["approver1_result"] = "Rejected";
        vals["approver1_opinion"] = opinion;
    }

    bool updOk = DatabaseManager::instance().update("records", vals,
        "id=:id", {{"id", m_currentRecordId}});
    if (updOk) {
        AuditService::instance().log(auth.currentUser().id, auth.currentUser().realName,
            AuditService::ACTION_REJECT, "审批中心", "Record", m_currentRecordId,
            "驳回笔录，原因:" + reason);
        QMessageBox::information(this, "已驳回", "笔录已驳回给提交人");
        m_currentRecordId = 0;
        m_preview->clear();
        loadApprovals();
    }
}