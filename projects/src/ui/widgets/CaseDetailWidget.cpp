#include "ui/widgets/CaseDetailWidget.h"
#include "models/Case.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QMessageBox>
#include <QDebug>
#include <QGroupBox>

CaseDetailWidget::CaseDetailWidget(int caseId, QWidget* parent)
    : QWidget(parent)
    , m_caseId(caseId)
    , m_isNew(caseId <= 0)
    , m_readOnly(false)
{
    setupUi();
    if (!m_isNew) {
        loadCase(caseId);
    } else {
        clearFields();
    }
}

CaseDetailWidget::~CaseDetailWidget() = default;

void CaseDetailWidget::setupUi()
{
    setStyleSheet(R"(
        QDialog{background:#0D0D0D;border:1px solid #2A2A2A;border-radius:10px;}
        QLabel{color:#CACACA;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QGroupBox{font-weight:bold;font-size:13px;color:#5E6AD2;border:1px solid #2A2A2A;border-radius:8px;margin-top:10px;background:#141414;padding-top:10px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 8px;background:#141414;}
        QLineEdit{padding:8px 12px;border:1.5px solid #2A2A2A;border-radius:6px;background:#191919;color:#CACACA;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit:focus{border-color:#5E6AD2;}
        QTextEdit{padding:8px 12px;border:1.5px solid #2A2A2A;border-radius:6px;background:#191919;color:#CACACA;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QPushButton#saveBtn{background:#5E6AD2;color:#FFFFFF;font-weight:bold;border:none;border-radius:6px;padding:10px 24px;}
        QPushButton#saveBtn:hover{background:#6E7AE2;}
        QPushButton#deleteBtn{background:#f38ba8;color:#FFFFFF;border:none;border-radius:6px;padding:10px 24px;font-weight:bold;}
        QPushButton#deleteBtn:hover{background:#f5a0b0;}
        QPushButton#cancelBtn{background:#2A2A2A;color:#CACACA;border:none;border-radius:6px;padding:10px 24px;}
        QPushButton#cancelBtn:hover{background:#2E2E2E;}
    )");
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(12);

    // 标题
    m_titleLabel = new QLabel(m_isNew ? "➕ 新建案件" : "📂 案件详情");
    m_titleLabel->setStyleSheet("font-size:18px;font-weight:bold;color:#CACACA;");
    main->addWidget(m_titleLabel);

    // 表单
    QGroupBox* formBox = new QGroupBox("案件信息");
    QFormLayout* form = new QFormLayout(formBox);

    // 案号
    m_caseNumberEdit = new QLineEdit;
    m_caseNumberEdit->setMaxLength(50);
    form->addRow("案号 <span style='color:red'>*</span>:", m_caseNumberEdit);

    // 案件名称
    m_caseNameEdit = new QLineEdit;
    m_caseNameEdit->setMaxLength(200);
    form->addRow("案件名称 <span style='color:red'>*</span>:", m_caseNameEdit);

    // 案件类型
    m_caseTypeCombo = new QComboBox;
    m_caseTypeCombo->addItems(Case::CASE_TYPES);
    form->addRow("案件类型:", m_caseTypeCombo);

    // 案件状态
    m_statusCombo = new QComboBox;
    m_statusCombo->addItems({"🔵 侦查中", "🟡 审查起诉", "🟢 已结案"});
    form->addRow("案件状态:", m_statusCombo);

    // 立案日期
    m_filingDateEdit = new QDateEdit;
    m_filingDateEdit->setCalendarPopup(true);
    m_filingDateEdit->setDate(QDate::currentDate());
    form->addRow("立案日期:", m_filingDateEdit);

    // 承办人
    m_handlerEdit = new QLineEdit;
    m_handlerEdit->setMaxLength(50);
    form->addRow("承办人:", m_handlerEdit);

    // 摘要
    QLabel* summaryLabel = new QLabel("案件摘要:");
    form->addRow(summaryLabel, m_summaryEdit);

    m_summaryEdit = new QTextEdit;
    m_summaryEdit->setMaximumHeight(120);
    m_summaryEdit->setPlaceholderText("请输入案件摘要...");
    form->addRow(m_summaryEdit);

    main->addWidget(formBox);

    // 按钮行
    QHBoxLayout* btnRow = new QHBoxLayout;

    m_saveBtn = new QPushButton("💾 保存");
    m_saveBtn->setObjectName("saveBtn");
    m_saveBtn->setMaximumWidth(120);

    m_deleteBtn = new QPushButton("🗑️ 删除");
    m_deleteBtn->setObjectName("deleteBtn");

    m_cancelBtn = new QPushButton("❌ 取消");
    m_cancelBtn->setObjectName("cancelBtn");

    btnRow->addWidget(m_saveBtn);
    btnRow->addWidget(m_deleteBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_cancelBtn);
    main->addLayout(btnRow);

    // 初始状态
    if (m_isNew) {
        m_deleteBtn->setVisible(false);
    }

    // 信号
    connect(m_saveBtn, &QPushButton::clicked, this, &CaseDetailWidget::onSaveClicked);
    connect(m_deleteBtn, &QPushButton::clicked, this, &CaseDetailWidget::onDeleteClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CaseDetailWidget::onCancelClicked);
}

void CaseDetailWidget::loadCase(int caseId)
{
    auto result = DatabaseManager::instance().selectOne(
        "SELECT * FROM cases WHERE id=:id", {{":id", caseId}});

    if (result.isEmpty()) {
        QMessageBox::warning(this, "错误", "案件不存在！");
        return;
    }

    m_case.id = result["id"].toInt();
    m_case.caseNumber = result["case_number"].toString();
    m_case.caseName = result["case_name"].toString();
    m_case.caseType = result["case_type"].toString();
    m_case.status = result["status"].toString();
    m_case.handler = result["handler"].toString();
    m_case.summary = result["summary"].toString();
    m_case.createdBy = result["created_by"].toInt();
    m_case.createdAt = result["created_at"].toString();
    m_case.updatedAt = result["updated_at"].toString();

    populateFields();
}

void CaseDetailWidget::populateFields()
{
    m_caseNumberEdit->setText(m_case.caseNumber);
    m_caseNameEdit->setText(m_case.caseName);

    int typeIdx = Case::CASE_TYPES.indexOf(m_case.caseType);
    if (typeIdx >= 0) m_caseTypeCombo->setCurrentIndex(typeIdx);

    int statusIdx = 0;
    if (m_case.status == Case::STATUS_INVESTIGATION) statusIdx = 0;
    else if (m_case.status == Case::STATUS_PENDING_TRIAL) statusIdx = 1;
    else if (m_case.status == Case::STATUS_CLOSED) statusIdx = 2;
    m_statusCombo->setCurrentIndex(statusIdx);

    m_filingDateEdit->setDate(m_case.filingDate.isValid()
        ? m_case.filingDate : QDate::currentDate());
    m_handlerEdit->setText(m_case.handler);
    m_summaryEdit->setPlainText(m_case.summary);
}

void CaseDetailWidget::clearFields()
{
    m_caseNumberEdit->clear();
    m_caseNameEdit->clear();
    m_caseTypeCombo->setCurrentIndex(0);
    m_statusCombo->setCurrentIndex(0);
    m_filingDateEdit->setDate(QDate::currentDate());
    m_handlerEdit->clear();
    m_summaryEdit->clear();
}

bool CaseDetailWidget::validateInput()
{
    if (m_caseNumberEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "请输入案号！");
        m_caseNumberEdit->setFocus();
        return false;
    }
    if (m_caseNameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "验证失败", "请输入案件名称！");
        m_caseNameEdit->setFocus();
        return false;
    }
    return true;
}

void CaseDetailWidget::onSaveClicked()
{
    if (!validateInput()) return;

    QString statusValue;
    switch (m_statusCombo->currentIndex()) {
        case 0: statusValue = Case::STATUS_INVESTIGATION; break;
        case 1: statusValue = Case::STATUS_PENDING_TRIAL; break;
        case 2: statusValue = Case::STATUS_CLOSED; break;
    }

    QMap<QString, QVariant> values = {
        {"case_number", m_caseNumberEdit->text().trimmed()},
        {"case_name", m_caseNameEdit->text().trimmed()},
        {"case_type", m_caseTypeCombo->currentText()},
        {"status", statusValue},
        {"filing_date", m_filingDateEdit->date().toString("yyyy-MM-dd")},
        {"handler", m_handlerEdit->text().trimmed()},
        {"summary", m_summaryEdit->toPlainText().trimmed()},
        {"updated_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}
    };

    bool success;
    if (m_isNew) {
        values["created_by"] = AuthService::instance().currentUser().id;
        values["created_at"] = values["updated_at"];
        int newId = DatabaseManager::instance().insert("cases", values);
        success = newId > 0;
        if (success) {
            m_case.id = newId;
            m_case.createdBy = AuthService::instance().currentUser().id;
            AuditService::instance().log(
                AuthService::instance().currentUser().id,
                AuthService::instance().currentUser().realName,
                AuditService::ACTION_CREATE, "案件管理", "Case", newId,
                "新建案件:" + m_caseNameEdit->text());
        }
    } else {
        success = DatabaseManager::instance().update("cases", values,
            "id=:id", {{":id", m_case.id}});
        if (success) {
            AuditService::instance().log(
                AuthService::instance().currentUser().id,
                AuthService::instance().currentUser().realName,
                AuditService::ACTION_UPDATE, "案件管理", "Case", m_case.id,
                "编辑案件:" + m_caseNameEdit->text());
        }
    }

    if (success) {
        QMessageBox::information(this, "成功", m_isNew ? "案件已创建！" : "案件已保存！");
        m_case.caseName = m_caseNameEdit->text();
        m_case.caseNumber = m_caseNumberEdit->text();
        emit caseSaved(m_case);
        close();
    } else {
        QMessageBox::warning(this, "错误", "保存失败！");
    }
}

void CaseDetailWidget::onDeleteClicked()
{
    if (QMessageBox::warning(this, "确认删除",
        "确定要删除此案件吗？此操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    if (DatabaseManager::instance().remove("cases", "id=:id", {{":id", m_case.id}})) {
        AuditService::instance().log(
            AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_DELETE, "案件管理", "Case", m_case.id,
            "删除案件:" + m_case.caseName);
        QMessageBox::information(this, "成功", "案件已删除！");
        emit caseDeleted(m_case.id);
        close();
    } else {
        QMessageBox::warning(this, "错误", "删除失败！");
    }
}

void CaseDetailWidget::onCancelClicked()
{
    close();
}

void CaseDetailWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    m_caseNumberEdit->setReadOnly(readOnly);
    m_caseNameEdit->setReadOnly(readOnly);
    m_caseTypeCombo->setEnabled(!readOnly);
    m_statusCombo->setEnabled(!readOnly);
    m_filingDateEdit->setEnabled(!readOnly);
    m_handlerEdit->setReadOnly(readOnly);
    m_summaryEdit->setReadOnly(readOnly);
    m_saveBtn->setVisible(!readOnly);
    m_deleteBtn->setVisible(!readOnly);
}
