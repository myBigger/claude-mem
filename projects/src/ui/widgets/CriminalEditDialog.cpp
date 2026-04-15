#include "ui/widgets/CriminalEditDialog.h"
#include "database/DatabaseManager.h"
#include "utils/DateUtils.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDate>
#include <QDebug>

CriminalEditDialog::CriminalEditDialog(QWidget* parent, const QString& cid)
    : QDialog(parent), m_cid(cid) {
    setWindowTitle(cid.isEmpty() ? "新增罪犯档案" : "编辑罪犯档案");
    setMinimumSize(750, 680);
    setupUI();
    loadExistingData();
}

void CriminalEditDialog::setupUI() {
    setMinimumSize(900, 700);
    setStyleSheet(R"(
        QDialog{background:#0A0E14;border:1px solid #21262D;border-radius:10px;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit{padding:8px 12px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit:focus{border-color:#0066CC;}
        QLineEdit:disabled{background:#0A0E14;color:#6E7681;border-color:#21262D;}
        QComboBox{padding:6px 10px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QComboBox:focus{border-color:#0066CC;}
        QDateEdit{padding:6px 10px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QSpinBox{padding:6px 10px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QTextEdit{padding:8px 12px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QGroupBox{font-weight:600;font-size:12px;color:#0066CC;border:1px solid #21262D;border-radius:8px;margin-top:10px;background:#0F1419;padding-top:10px;text-transform:uppercase;letter-spacing:0.5px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 8px;background:#0F1419;}
        QFrame#btnFrame{background:#0F1419;border-top:1px solid #21262D;border-radius:0 0 10px 10px;}
        QPushButton#saveBtn{background:#0066CC;color:#FFFFFF;font-weight:600;font-size:14px;border:none;border-radius:4px;padding:8px 24px;}
        QPushButton#saveBtn:hover{background:#0052A3;}
        QPushButton#cancelBtn{background:#1C2128;color:#E6EDF3;font-size:14px;border:1px solid #21262D;border-radius:4px;padding:8px 24px;}
        QPushButton#cancelBtn:hover{background:#30363D;}
    )");

    // === 基础信息 ===
    m_idEdit = new QLineEdit;
    m_idEdit->setReadOnly(true);

    QLineEdit* nameEdit = new QLineEdit;
    m_fields["name"] = nameEdit;

    QComboBox* genderCombo = new QComboBox;
    genderCombo->addItems({"男","女"});
    m_fields["gender"] = genderCombo;

    QLineEdit* ethnicityEdit = new QLineEdit;
    m_fields["ethnicity"] = ethnicityEdit;

    QDateEdit* birthEdit = new QDateEdit;
    birthEdit->setCalendarPopup(true);
    birthEdit->setDisplayFormat("yyyy-MM-dd");
    m_fields["birth_date"] = birthEdit;

    QLineEdit* idCardEdit = new QLineEdit;
    idCardEdit->setMaxLength(18);
    m_fields["id_card_number"] = idCardEdit;
    connect(idCardEdit, &QLineEdit::textChanged, this, &CriminalEditDialog::onIdCardChanged);

    QLineEdit* nativeEdit = new QLineEdit;
    m_fields["native_place"] = nativeEdit;

    QComboBox* eduCombo = new QComboBox;
    eduCombo->addItems({"文盲","小学","初中","高中","中专","大专","本科","硕士","博士"});
    m_fields["education"] = eduCombo;

    // === 案件信息 ===
    QLineEdit* crimeEdit = new QLineEdit;
    m_fields["crime"] = crimeEdit;

    QSpinBox* yearsSpin = new QSpinBox;
    yearsSpin->setRange(0, 50);
    yearsSpin->setSuffix(" 年");
    yearsSpin->setValue(0);
    QSpinBox* monthsSpin = new QSpinBox;
    monthsSpin->setRange(0, 11);
    monthsSpin->setSuffix(" 月");
    monthsSpin->setValue(0);
    QHBoxLayout* sentLayout = new QHBoxLayout;
    sentLayout->addWidget(yearsSpin);
    sentLayout->addWidget(monthsSpin);
    m_fields["sentence_years"] = yearsSpin;
    m_fields["sentence_months"] = monthsSpin;

    QDateEdit* entryEdit = new QDateEdit;
    entryEdit->setCalendarPopup(true);
    entryEdit->setDisplayFormat("yyyy-MM-dd");
    entryEdit->setDate(QDate::currentDate());
    m_fields["entry_date"] = entryEdit;

    QLineEdit* courtEdit = new QLineEdit;
    m_fields["original_court"] = courtEdit;

    QComboBox* distCombo = new QComboBox;
    distCombo->addItems({"一监区","二监区","三监区","四监区","五监区"});
    m_fields["district"] = distCombo;

    QLineEdit* cellEdit = new QLineEdit;
    m_fields["cell"] = cellEdit;

    QComboBox* levelCombo = new QComboBox;
    levelCombo->addItems({"A","B","C"});
    m_fields["manage_level"] = levelCombo;

    QComboBox* crimeTypeCombo = new QComboBox;
    crimeTypeCombo->addItems({"暴力型","财产型","毒品型","职务型","其他"});
    m_fields["crime_type"] = crimeTypeCombo;

    QLineEdit* handlerEdit = new QLineEdit;
    handlerEdit->setReadOnly(true);
    handlerEdit->setText(AuthService::instance().currentUser().userId);
    m_fields["handler_id"] = handlerEdit;

    QTextEdit* remarkEdit = new QTextEdit;
    remarkEdit->setMaximumHeight(80);
    m_fields["remark"] = remarkEdit;

    // === 双列网格布局 ===
    // 左列：基础信息 | 右列：案件信息
    QGroupBox* leftBox = new QGroupBox("基础信息");
    QGridLayout* leftGrid = new QGridLayout(leftBox);
    leftGrid->setContentsMargins(12, 15, 12, 12);
    leftGrid->setHorizontalSpacing(10);
    leftGrid->setVerticalSpacing(8);
    leftGrid->setColumnStretch(0, 0);
    leftGrid->setColumnStretch(1, 1);

    auto addLeftRow = [&](int row, const QString& label, QWidget* field) {
        QLabel* lb = new QLabel(label);
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        leftGrid->addWidget(lb, row, 0, Qt::AlignRight);
        leftGrid->addWidget(field, row, 1, Qt::AlignLeft);
    };

    addLeftRow(0, "罪犯编号：", m_idEdit);
    addLeftRow(1, "姓名：", nameEdit);
    addLeftRow(2, "性别：", genderCombo);
    addLeftRow(3, "民族：", ethnicityEdit);
    addLeftRow(4, "出生日期：", birthEdit);
    addLeftRow(5, "身份证号：", idCardEdit);
    addLeftRow(6, "籍贯：", nativeEdit);
    addLeftRow(7, "文化程度：", eduCombo);

    QGroupBox* rightBox = new QGroupBox("案件信息");
    QGridLayout* rightGrid = new QGridLayout(rightBox);
    rightGrid->setContentsMargins(12, 15, 12, 12);
    rightGrid->setHorizontalSpacing(10);
    rightGrid->setVerticalSpacing(8);
    rightGrid->setColumnStretch(0, 0);
    rightGrid->setColumnStretch(1, 1);

    auto addRightRow = [&](int row, const QString& label, QWidget* field) {
        QLabel* lb = new QLabel(label);
        lb->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rightGrid->addWidget(lb, row, 0, Qt::AlignRight);
        rightGrid->addWidget(field, row, 1, Qt::AlignLeft);
    };

    addRightRow(0, "罪名：", crimeEdit);
    addRightRow(1, "刑期：", yearsSpin);
    // 单独放月份
    QWidget* sentRow = new QWidget;
    QHBoxLayout* sentHL = new QHBoxLayout(sentRow);
    sentHL->setContentsMargins(0,0,0,0);
    sentHL->addWidget(yearsSpin);
    sentHL->addWidget(monthsSpin);
    rightGrid->addWidget(new QLabel("刑期："), 1, 0, Qt::AlignRight);
    rightGrid->addWidget(sentRow, 1, 1, Qt::AlignLeft);
    addRightRow(2, "入狱日期：", entryEdit);
    addRightRow(3, "原判法院：", courtEdit);
    addRightRow(4, "所在监区：", distCombo);
    addRightRow(5, "所在监室：", cellEdit);
    addRightRow(6, "管理等级：", levelCombo);
    addRightRow(7, "涉案类型：", crimeTypeCombo);
    addRightRow(8, "主治管教：", handlerEdit);

    // 主布局：双列卡片 + 备注 + 按钮
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(15, 12, 15, 12);
    main->setSpacing(10);

    QHBoxLayout* cardLayout = new QHBoxLayout;
    cardLayout->setSpacing(12);
    cardLayout->addWidget(leftBox, 1);
    cardLayout->addWidget(rightBox, 1);
    main->addLayout(cardLayout);

    // 备注区
    QGroupBox* remarkBox = new QGroupBox("备注");
    QVBoxLayout* remarkLayout = new QVBoxLayout(remarkBox);
    remarkLayout->setContentsMargins(12, 12, 12, 8);
    remarkLayout->addWidget(remarkEdit);
    main->addWidget(remarkBox);

    // 按钮栏
    QFrame* btnFrame = new QFrame;
    btnFrame->setObjectName("btnFrame");
    btnFrame->setFixedHeight(52);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnFrame);
    btnLayout->setContentsMargins(15, 8, 15, 8);
    QPushButton* saveBtn = new QPushButton("保存");
    saveBtn->setObjectName("saveBtn");
    saveBtn->setFixedSize(110, 34);
    QPushButton* cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setFixedSize(110, 34);
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    main->addWidget(btnFrame);

    connect(saveBtn, &QPushButton::clicked, this, &CriminalEditDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void CriminalEditDialog::loadExistingData() {
    if (m_cid.isEmpty()) {
        m_idEdit->setText(DateUtils::generateId("FZ"));
        return;
    }
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM criminals WHERE criminal_id=:id", {{"id", m_cid}});
    if (!r.isEmpty()) {
        m_idEdit->setText(r["criminal_id"].toString());
        populateFields(r);
    }
}

void CriminalEditDialog::populateFields(const QMap<QString,QVariant>& data) {
    for (auto it = m_fields.constBegin(); it != m_fields.constEnd(); ++it) {
        QString key = it.key();
        if (!data.contains(key)) continue;
        QWidget* w = it.value();
        QVariant val = data[key];
        if (auto le = qobject_cast<QLineEdit*>(w)) {
            if (w != m_idEdit && !le->isReadOnly()) le->setText(val.toString());
        } else if (auto cb = qobject_cast<QComboBox*>(w)) {
            int idx = cb->findText(val.toString());
            if (idx >= 0) cb->setCurrentIndex(idx);
        } else if (auto de = qobject_cast<QDateEdit*>(w)) {
            if (val.canConvert<QString>()) {
                QDate d = QDate::fromString(val.toString(), "yyyy-MM-dd");
                if (d.isValid()) de->setDate(d);
            }
        } else if (auto sp = qobject_cast<QSpinBox*>(w)) {
            sp->setValue(val.toInt());
        } else if (auto te = qobject_cast<QTextEdit*>(w)) {
            te->setPlainText(val.toString());
        }
    }
}

void CriminalEditDialog::onIdCardChanged() {
    // 从身份证号自动提取出生日期和性别
    QString idCard = qobject_cast<QLineEdit*>(m_fields["id_card_number"])->text();
    if (idCard.length() >= 17) {
        // 提取性别：第17位，奇数为男，偶数为女
        int genderCode = idCard[16].digitValue();
        QComboBox* genderCombo = qobject_cast<QComboBox*>(m_fields["gender"]);
        genderCombo->setCurrentIndex(genderCode % 2 == 1 ? 0 : 1);

        // 提取出生日期
        QString birthStr = idCard.mid(6, 4) + "-" + idCard.mid(10, 2) + "-" + idCard.mid(12, 2);
        QDateEdit* birthEdit = qobject_cast<QDateEdit*>(m_fields["birth_date"]);
        QDate birthDate = QDate::fromString(birthStr, "yyyy-MM-dd");
        if (birthDate.isValid()) birthEdit->setDate(birthDate);
    }
}

bool CriminalEditDialog::validate() {
    QString name = qobject_cast<QLineEdit*>(m_fields["name"])->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "校验失败", "姓名不能为空");
        return false;
    }
    QString idCard = qobject_cast<QLineEdit*>(m_fields["id_card_number"])->text().trimmed();
    if (idCard.length() > 0 && idCard.length() != 18) {
        QMessageBox::warning(this, "校验失败", "身份证号必须为18位");
        return false;
    }
    return true;
}

void CriminalEditDialog::onSave() {
    if (!validate()) return;

    QMap<QString, QVariant> vals;
    for (auto it = m_fields.constBegin(); it != m_fields.constEnd(); ++it) {
        if (it.key() == "handler_id") continue; // 不允许改主治管教
        QWidget* w = it.value();
        if (auto le = qobject_cast<QLineEdit*>(w)) {
            if (!le->isReadOnly()) vals[it.key()] = le->text().trimmed();
        } else if (auto cb = qobject_cast<QComboBox*>(w)) {
            vals[it.key()] = cb->currentText();
        } else if (auto de = qobject_cast<QDateEdit*>(w)) {
            vals[it.key()] = de->date().toString("yyyy-MM-dd");
        } else if (auto sp = qobject_cast<QSpinBox*>(w)) {
            vals[it.key()] = sp->value();
        } else if (auto te = qobject_cast<QTextEdit*>(w)) {
            vals[it.key()] = te->toPlainText().trimmed();
        }
    }

    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    vals["updated_at"] = now;

    auto& auth = AuthService::instance();
    auto& audit = AuditService::instance();

    if (m_cid.isEmpty()) {
        // 新增
        vals["criminal_id"] = DateUtils::generateId("FZ");
        vals["created_at"] = now;
        vals["archived"] = 0;
        int newId = DatabaseManager::instance().insert("criminals", vals);
        if (newId > 0) {
            audit.log(auth.currentUser().id, auth.currentUser().realName,
                AuditService::ACTION_CREATE, "罪犯管理", "Criminal", newId,
                "新增罪犯档案:" + vals["criminal_id"].toString());
            QMessageBox::information(this, "成功", "罪犯档案已保存\n罪犯编号：" + vals["criminal_id"].toString());
            accept();
        } else {
            QMessageBox::warning(this, "失败", "保存失败，可能原因：\n1. 身份证号已存在\n2. 数据库写入错误");
        }
    } else {
        // 编辑
        vals["updated_at"] = now;
        bool ok = DatabaseManager::instance().update("criminals", vals,
            "criminal_id=:id", {{"id", m_cid}});
        if (ok) {
            audit.log(auth.currentUser().id, auth.currentUser().realName,
                AuditService::ACTION_UPDATE, "罪犯管理", "Criminal", 0,
                "编辑罪犯档案:" + m_cid);
            QMessageBox::information(this, "成功", "档案已更新");
            accept();
        } else {
            QMessageBox::warning(this, "失败", "更新失败，请重试");
        }
    }
}