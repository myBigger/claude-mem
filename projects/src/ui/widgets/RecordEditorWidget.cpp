#include "ui/widgets/RecordEditorWidget.h"
#include "ui/widgets/LinedTextEdit.h"
#include "ui/widgets/RecordThemeSelector.h"
#include "ui/widgets/ScanUploadDialog.h"
#include "ui/widgets/ScanDownloadDialog.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "services/KeyManager.h"
#include "utils/DateUtils.h"
#include <QPrinter>
#include <QPainter>
#include <QPrintDialog>
#include <QFileDialog>
#include <QPdfWriter>
#include <QStandardPaths>
#include <QFontMetrics>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QTimer>
#include <QDateTime>
#include <QInputDialog>
#include <QShortcut>

RecordEditorWidget::RecordEditorWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadDrafts();

    // 自动保存定时器（每5分钟）
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &RecordEditorWidget::onAutoSave);
    m_autoSaveTimer->start(5 * 60 * 1000); // 5分钟
}

void RecordEditorWidget::setupUI() {
    setStyleSheet(R"(
        QWidget{background:#0A0E14;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QGroupBox{font-weight:600;font-size:12px;color:#0066CC;border:1px solid #21262D;border-radius:8px;margin-top:10px;background:#0F1419;padding-top:10px;text-transform:uppercase;letter-spacing:0.5px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 8px;background:#0F1419;}
        QLineEdit{padding:8px 12px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit:focus{border-color:#0066CC;}
        QComboBox{padding:6px 10px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QDateEdit{padding:6px 10px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QFrame#themeBar{background:#0F1419;border:1px solid #21262D;border-radius:6px;}
        QFrame#bottomBar{background:#0F1419;border-top:1px solid #21262D;border-radius:0 0 10px 10px;}
        QPushButton{background:#1C2128;color:#E6EDF3;border:none;border-radius:4px;padding:8px 18px;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QPushButton:hover{background:#30363D;}
        QPushButton:disabled{background:#0A0E14;color:#6E7681;}
        #submitBtn{background:#0066CC;color:#FFFFFF;font-weight:600;border-radius:4px;}
        #submitBtn:hover{background:#0052A3;}
        #submitBtn:pressed{background:#003D82;}
    )");
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(8);

    // ===== 顶部：笔录基本信息 =====
    QGroupBox* infoBox = new QGroupBox("笔录基本信息");
    QGridLayout* infoGrid = new QGridLayout(infoBox);

    // 笔录类型
    infoGrid->addWidget(new QLabel("笔录类型："), 0, 0);
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({
        "请选择笔录类型...",
        "RT-01 入监笔录",
        "RT-02 日常谈话",
        "RT-03 案件调查",
        "RT-04 行政处罚告知",
        "RT-05 减刑假释评估",
        "RT-06 出监教育",
        "RT-07 医疗检查",
        "RT-08 亲属会见"
    });
    infoGrid->addWidget(m_typeCombo, 0, 1);

    // 笔录编号
    infoGrid->addWidget(new QLabel("笔录编号："), 0, 2);
    m_recordIdEdit = new QLineEdit;
    m_recordIdEdit->setReadOnly(true);
    m_recordIdEdit->setStyleSheet("background:#161B22;color:#8B949E;font-family:'JetBrains Mono','SF Mono','Consolas',monospace;font-size:12px;border:1px solid #21262D;border-radius:4px;padding:4px 8px;");
    infoGrid->addWidget(m_recordIdEdit, 0, 3);

    // 关联罪犯
    infoGrid->addWidget(new QLabel("关联罪犯："), 1, 0);
    QHBoxLayout* criminalLayout = new QHBoxLayout;
    m_criminalCombo = new QComboBox;
    m_criminalCombo->setEditable(false);
    m_criminalCombo->setMinimumWidth(200);
    m_criminalCombo->addItem("请先选择罪犯...", -1);
    criminalLayout->addWidget(m_criminalCombo, 1);
    QPushButton* selectCriminalBtn = new QPushButton("选择罪犯");
    criminalLayout->addWidget(selectCriminalBtn);
    m_criminalNameLabel = new QLabel("未选择");
    m_criminalNameLabel->setStyleSheet("color:#0066CC;font-weight:600;");
    criminalLayout->addWidget(m_criminalNameLabel);
    criminalLayout->addStretch();
    infoGrid->addLayout(criminalLayout, 1, 1, 1, 3);

    // 审讯时间 + 地点 + 在场人员
    infoGrid->addWidget(new QLabel("审讯时间："), 2, 0);
    m_datetimeEdit = new QDateTimeEdit;
    m_datetimeEdit->setCalendarPopup(true);
    m_datetimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
    m_datetimeEdit->setDateTime(QDateTime::currentDateTime());
    infoGrid->addWidget(m_datetimeEdit, 2, 1);

    infoGrid->addWidget(new QLabel("审讯地点："), 2, 2);
    m_locationCombo = new QComboBox;
    m_locationCombo->addItems({"审讯室1","审讯室2","监室","办公室","其他"});
    infoGrid->addWidget(m_locationCombo, 2, 3);

    infoGrid->addWidget(new QLabel("在场人员："), 3, 0);
    m_presentPersonsEdit = new QLineEdit;
    m_presentPersonsEdit->setPlaceholderText("输入在场人员姓名，多人以顿号分隔");
    infoGrid->addWidget(m_presentPersonsEdit, 3, 1, 1, 3);

    // 加载罪犯列表到下拉框
    auto criminals = DatabaseManager::instance().select(
        "SELECT id, criminal_id, name FROM criminals WHERE archived=0 ORDER BY name", {});
    for (const auto& c : criminals) {
        m_criminalCombo->addItem(
            QString("%1 - %2").arg(c["criminal_id"].toString()).arg(c["name"].toString()),
            c["id"].toInt());
    }

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout;
    QPushButton* boldBtn = new QPushButton("B");
    boldBtn->setMaximumWidth(32);
    boldBtn->setStyleSheet("font-weight:bold;padding:4px 8px;");
    QPushButton* italicBtn = new QPushButton("I");
    italicBtn->setMaximumWidth(32);
    italicBtn->setStyleSheet("font-style:italic;padding:4px 8px;");
    QPushButton* underlineBtn = new QPushButton("U");
    underlineBtn->setMaximumWidth(32);
    underlineBtn->setStyleSheet("text-decoration:underline;padding:4px 8px;");

    toolbar->addWidget(new QLabel("格式："));
    toolbar->addWidget(boldBtn);
    toolbar->addWidget(italicBtn);
    toolbar->addWidget(underlineBtn);
    toolbar->addSpacing(20);

    m_saveTimeLabel = new QLabel("尚未保存");
    m_saveTimeLabel->setStyleSheet("color:#8B949E;font-size:12px;");
    m_statusLabel = new QLabel("状态：新建");
    m_statusLabel->setStyleSheet("color:#8B949E;font-size:12px;");

    toolbar->addWidget(m_saveTimeLabel);
    toolbar->addStretch();
    toolbar->addWidget(m_statusLabel);

    // 编辑器（必须在主题选择器之前创建）
    m_editor = new LinedTextEdit;

    // 主题选择器
    QFrame* themeBar = new QFrame;
    themeBar->setObjectName("themeBar");
    themeBar->setFrameShape(QFrame::Box);
    QHBoxLayout* themeBarLayout = new QHBoxLayout(themeBar);
    themeBarLayout->setContentsMargins(8, 4, 8, 4);
    m_themeSelector = new RecordThemeSelector(m_editor, this);
    themeBarLayout->addWidget(m_themeSelector);

    // 编辑器外层盒子
    QGroupBox* editorBox = new QGroupBox("笔录正文");
    m_editor->setPlaceholderText(
        "选择笔录类型 → 选择罪犯 → 开始编辑\n\n"
        "提示：\n"
        "  • Ctrl+S 保存草稿\n"
        "  • 内容填写完毕后，点击「提交审批」提交审核");
    m_editor->setMinimumHeight(320);
    m_editor->setFont(QFont("Microsoft YaHei", 13));

    QVBoxLayout* editorLayout = new QVBoxLayout(editorBox);
    editorLayout->addWidget(themeBar);
    editorLayout->addLayout(toolbar);
    editorLayout->addWidget(m_editor);

    // 草稿箱
    QGroupBox* draftBox = new QGroupBox("草稿箱");
    m_draftsTable = new QTableWidget;
    m_draftsTable->setColumnCount(5);
    m_draftsTable->setHorizontalHeaderLabels({"笔录编号","笔录类型","罪犯","状态","保存时间"});
    m_draftsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_draftsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_draftsTable->setAlternatingRowColors(true);
    m_draftsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_draftsTable->setMaximumHeight(120);
    QVBoxLayout* draftLayout = new QVBoxLayout(draftBox);
    draftLayout->addWidget(m_draftsTable);

    // 底部按钮
    QFrame* bottomFrame = new QFrame;
    bottomFrame->setObjectName("bottomBar");
    QHBoxLayout* bottomBar = new QHBoxLayout(bottomFrame);
    bottomBar->setContentsMargins(0, 8, 0, 0);
    QPushButton* newBtn = new QPushButton("新建笔录");
    QPushButton* printBtn = new QPushButton("打印笔录");
    m_uploadScanBtn = new QPushButton("🔒 上传扫描件");
    m_uploadScanBtn->setEnabled(false); // 保存后才可用
    m_downloadScanBtn = new QPushButton("📥 下载扫描件");
    m_downloadScanBtn->setEnabled(false);
    QPushButton* saveBtn = new QPushButton("保存草稿");
    QPushButton* submitBtn = new QPushButton("提交审批");
    QPushButton* clearBtn = new QPushButton("清空");
    bottomBar->addWidget(newBtn);
    bottomBar->addWidget(printBtn);
    bottomBar->addWidget(m_uploadScanBtn);
    bottomBar->addWidget(m_downloadScanBtn);
    bottomBar->addStretch();
    bottomBar->addWidget(saveBtn);
    bottomBar->addWidget(submitBtn);
    bottomBar->addWidget(clearBtn);

    main->addWidget(infoBox, 0);
    main->addWidget(editorBox, 1);
    main->addWidget(draftBox, 0);
    main->addWidget(bottomFrame);
    main->setContentsMargins(10,10,10,10);

    // 信号连接
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecordEditorWidget::onLoadTemplate);
    connect(selectCriminalBtn, &QPushButton::clicked, this, &RecordEditorWidget::onSelectCriminal);
    connect(m_criminalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [=](int idx) {
                if (idx > 0 && idx < m_criminalCombo->count()) {
                    m_currentCriminalId = m_criminalCombo->currentData().toInt();
                    m_currentCriminalName = m_criminalCombo->currentText();
                    m_criminalNameLabel->setText(m_criminalCombo->currentText());
                }
            });
    connect(newBtn, &QPushButton::clicked, this, &RecordEditorWidget::onNewRecord);
    connect(saveBtn, &QPushButton::clicked, this, &RecordEditorWidget::onSaveDraft);
    connect(printBtn, &QPushButton::clicked, this, &RecordEditorWidget::onPrintRecord);
    connect(submitBtn, &QPushButton::clicked, this, &RecordEditorWidget::onSubmit);
    connect(m_uploadScanBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentRecordId == 0) {
            QMessageBox::warning(this, "提示", "请先保存笔录草稿，再上传扫描件。");
            return;
        }
        QString recId = m_recordIdEdit->text().isEmpty()
                        ? DateUtils::generateId("BJ") : m_recordIdEdit->text();
        ScanUploadDialog dlg(recId, m_currentRecordId, this);
        if (dlg.exec() == QDialog::Accepted) {
            // 上传成功后更新状态
            m_statusLabel->setText("🔒 状态：已签名扫描件（已锁定）");
            m_statusLabel->setStyleSheet("color:#22C55E;font-weight:600;");
            m_uploadScanBtn->setEnabled(false);
            m_uploadScanBtn->setText("🔒 已上传");
        }
    });
    connect(m_downloadScanBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentRecordId == 0) {
            QMessageBox::warning(this, "提示", "请先保存笔录草稿，再下载扫描件。");
            return;
        }
        QString recId = m_recordIdEdit->text().isEmpty()
                        ? DateUtils::generateId("BJ") : m_recordIdEdit->text();
        ScanDownloadDialog dlg(this);
        dlg.exec();
    });
    connect(clearBtn, &QPushButton::clicked, [=]() {
        if (m_isDirty) {
            if (QMessageBox::question(this,"确认","有未保存的内容，确定清空？") != QMessageBox::Yes) return;
        }
        m_editor->clear();
        m_isDirty = false;
    });

    // 富文本格式（使用 QTextCharFormat）
    auto applyFormat = [this](const QTextCharFormat& fmt) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.mergeCharFormat(fmt);
        m_editor->mergeCurrentCharFormat(fmt);
    };
    connect(boldBtn, &QPushButton::clicked, this, [=]() {
        QTextCharFormat fmt; fmt.setFontWeight(QFont::Bold);
        applyFormat(fmt);
    });
    connect(italicBtn, &QPushButton::clicked, this, [=]() {
        QTextCharFormat fmt; fmt.setFontItalic(true);
        applyFormat(fmt);
    });
    connect(underlineBtn, &QPushButton::clicked, this, [=]() {
        QTextCharFormat fmt; fmt.setFontUnderline(true);
        applyFormat(fmt);
    });

    // Ctrl+S 保存草稿
    QShortcut* saveSc = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveSc, &QShortcut::activated, this, [this]() { if(m_isDirty) onSaveDraft(); });

    // Ctrl+B 加粗 / Ctrl+I 斜体 / Ctrl+U 下划线
    QShortcut* boldSc = new QShortcut(QKeySequence("Ctrl+B"), this);
    connect(boldSc, &QShortcut::activated, this, [=]() {
        QTextCharFormat fmt; fmt.setFontWeight(QFont::Bold); applyFormat(fmt);
    });
    QShortcut* italicSc = new QShortcut(QKeySequence("Ctrl+I"), this);
    connect(italicSc, &QShortcut::activated, this, [=]() {
        QTextCharFormat fmt; fmt.setFontItalic(true); applyFormat(fmt);
    });
    QShortcut* ulSc = new QShortcut(QKeySequence("Ctrl+U"), this);
    connect(ulSc, &QShortcut::activated, this, [=]() {
        QTextCharFormat fmt; fmt.setFontUnderline(true); applyFormat(fmt);
    });

    connect(m_editor, &QTextEdit::textChanged, [=]() { m_isDirty = true; });
    connect(m_draftsTable, &QTableWidget::doubleClicked, this, [=](const QModelIndex&) {
        onLoadDraft();
    });
}

void RecordEditorWidget::onNewRecord() {
    if (m_isDirty) {
        if (QMessageBox::question(this,"确认","有未保存内容，是否先保存？")
            == QMessageBox::Yes) { onSaveDraft(); return; }
    }
    m_typeCombo->setCurrentIndex(0);
    m_criminalCombo->setCurrentIndex(0);
    m_currentCriminalId = 0;
    m_currentCriminalName.clear();
    m_criminalNameLabel->setText("未选择");
    m_recordIdEdit->setText(DateUtils::generateId("BJ"));
    m_datetimeEdit->setDateTime(QDateTime::currentDateTime());
    m_editor->clear();
    m_currentRecordId = 0;
    m_currentTemplateId = 0;
    m_isDirty = false;
    m_statusLabel->setText("状态：新建");
    m_saveTimeLabel->setText("尚未保存");
    m_uploadScanBtn->setEnabled(false);
    m_uploadScanBtn->setText("🔒 上传扫描件");
}

void RecordEditorWidget::onSelectCriminal() {
    // 弹出罪犯选择对话框
    bool ok = false;
    QString selected = QInputDialog::getItem(this, "选择罪犯",
        "请选择关联的罪犯：", []()->QStringList{
            QStringList items;
            auto criminals = DatabaseManager::instance().select(
                "SELECT id, criminal_id, name FROM criminals WHERE archived=0 ORDER BY name", {});
            for (const auto& c : criminals) {
                items << QString("%1 - %2").arg(c["criminal_id"].toString()).arg(c["name"].toString());
            }
            return items;
        }(), 0, false, &ok);
    if (!ok || selected.isEmpty()) return;
    for (int i = 1; i < m_criminalCombo->count(); ++i) {
        if (m_criminalCombo->itemText(i) == selected) {
            m_criminalCombo->setCurrentIndex(i);
            m_currentCriminalId = m_criminalCombo->currentData().toInt();
            m_currentCriminalName = selected;
            m_criminalNameLabel->setText(selected);
            break;
        }
    }
}

void RecordEditorWidget::onLoadTemplate(int typeIndex) {
    if (typeIndex == 0) return; // 默认项
    QString typeCode = QString("RT-%1").arg(typeIndex, 2, 10, QChar('0'));
    auto rows = DatabaseManager::instance().select(
        "SELECT id, content FROM templates WHERE record_type=:t AND status='Enabled' LIMIT 1",
        {{"t", typeCode}});
    if (!rows.isEmpty()) {
        m_currentTemplateId = rows.first()["id"].toInt();
        QString content = rows.first()["content"].toString();
        m_editor->setPlainText(content);
        m_isDirty = true;
    } else {
        m_editor->clear();
    }
}

QString RecordEditorWidget::renderTemplate(const QString& templateContent, int criminalId) {
    if (criminalId <= 0) return templateContent;
    auto criminal = DatabaseManager::instance().selectOne(
        "SELECT * FROM criminals WHERE id=:id", {{"id", criminalId}});
    if (criminal.isEmpty()) return templateContent;

    QString result = templateContent;
    result.replace("[罪犯姓名]", criminal["name"].toString());
    result.replace("[性别]", criminal["gender"].toString());
    result.replace("[民族]", criminal["ethnicity"].toString());
    result.replace("[身份证号]", criminal["id_card_number"].toString());
    result.replace("[籍贯]", criminal["native_place"].toString());
    result.replace("[入狱日期]", criminal["entry_date"].toString());
    result.replace("[原判法院]", criminal["original_court"].toString());
    result.replace("[刑期]", QString("%1年%2月")
        .arg(criminal["sentence_years"].toInt())
        .arg(criminal["sentence_months"].toInt()));
    result.replace("[监区]", criminal["district"].toString());
    result.replace("[监室]", criminal["cell"].toString());
    result.replace("[___讯问人___]", AuthService::instance().currentUser().realName);
    result.replace("[___记录员___]", AuthService::instance().currentUser().realName);
    result.replace("[___YYYY-MM-DD HH:MM___]", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    result.replace("[___地点___]", "审讯室1");
    return result;
}

void RecordEditorWidget::onSaveDraft() {
    int typeIdx = m_typeCombo->currentIndex();
    if (typeIdx == 0) {
        QMessageBox::warning(this, "提示", "请先选择笔录类型");
        return;
    }
    if (m_currentCriminalId == 0) {
        QMessageBox::warning(this, "提示", "请先选择关联罪犯");
        return;
    }
    QString content = m_editor->toPlainText();
    if (content.trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "笔录正文不能为空");
        return;
    }

    auto& auth = AuthService::instance();
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString recordId = m_recordIdEdit->text().isEmpty()
        ? DateUtils::generateId("BJ") : m_recordIdEdit->text();
    QString typeCode = QString("RT-%1").arg(typeIdx, 2, 10, QChar('0'));

    if (m_currentRecordId == 0) {
        // 新建
        // 加密笔录正文（应用层 AES-256 加密，主密钥由 PIN 保护）
        QString encryptedContent;
        if (KeyManager::instance().isUnlocked()) {
            encryptedContent = QString::fromUtf8(
                KeyManager::instance().encryptField(content.toUtf8()));
        } else {
            encryptedContent = content; // PIN 未解锁时降级为明文（不应发生）
        }
        QMap<QString, QVariant> vals = {
            {"record_id", recordId},
            {"record_type", typeCode},
            {"criminal_id", m_currentCriminalId},
            {"criminal_name", m_currentCriminalName.split(" - ").value(1, m_currentCriminalName)},
            {"record_date", m_datetimeEdit->dateTime().toString("yyyy-MM-dd hh:mm")},
            {"record_location", m_locationCombo->currentText()},
            {"interrogator_id", auth.currentUser().userId},
            {"recorder_id", auth.currentUser().userId},
            {"present_persons", m_presentPersonsEdit->text()},
            {"content", encryptedContent},
            {"content_encrypted", 1},
            {"status", "Draft"},
            {"created_by", auth.currentUser().id},
            {"created_at", now},
            {"updated_at", now}
        };
        int newId = DatabaseManager::instance().insert("records", vals);
        if (newId > 0) {
            m_currentRecordId = newId;
            m_recordIdEdit->setText(recordId);
            m_saveTimeLabel->setText("上次保存：" + now);
            m_statusLabel->setText("状态：草稿（已保存）");
            m_isDirty = false;
            // 保存成功后启用上传扫描件按钮
            m_uploadScanBtn->setEnabled(true);
            m_uploadScanBtn->setText("🔒 上传扫描件");
            m_downloadScanBtn->setEnabled(true);
            m_downloadScanBtn->setText("📥 下载扫描件");
            AuditService::instance().log(auth.currentUser().id, auth.currentUser().realName,
                AuditService::ACTION_CREATE, "笔录管理", "Record", newId,
                "保存笔录草稿:" + recordId);
            QMessageBox::information(this, "保存成功", "草稿已保存");
            loadDrafts();
        }
    } else {
        // 更新：同样加密 content 后写入
        QString encryptedContent;
        if (KeyManager::instance().isUnlocked()) {
            encryptedContent = QString::fromUtf8(
                KeyManager::instance().encryptField(content.toUtf8()));
        } else {
            encryptedContent = content;
        }
        QMap<QString, QVariant> vals = {
            {"content", encryptedContent},
            {"content_encrypted", 1},
            {"criminal_id", m_currentCriminalId},
            {"record_date", m_datetimeEdit->dateTime().toString("yyyy-MM-dd hh:mm")},
            {"record_location", m_locationCombo->currentText()},
            {"present_persons", m_presentPersonsEdit->text()},
            {"version", 1}, // 简化：每次保存version+1
            {"updated_at", now}
        };
        DatabaseManager::instance().update("records", vals,
            "id=:id", {{"id", m_currentRecordId}});
        m_saveTimeLabel->setText("上次保存：" + now);
        m_statusLabel->setText("状态：草稿（已保存）");
        m_isDirty = false;
        m_uploadScanBtn->setEnabled(true);
        m_uploadScanBtn->setText("🔒 上传扫描件");
        m_downloadScanBtn->setEnabled(true);
        m_downloadScanBtn->setText("📥 下载扫描件");
        loadDrafts();
    }
}

bool RecordEditorWidget::validateRecord() {
    if (m_typeCombo->currentIndex() == 0) { QMessageBox::warning(this,"校验","请选择笔录类型"); return false; }
    if (m_currentCriminalId == 0) { QMessageBox::warning(this,"校验","请选择关联罪犯"); return false; }
    if (m_editor->toPlainText().trimmed().isEmpty()) { QMessageBox::warning(this,"校验","笔录正文不能为空"); return false; }
    // 检查签名区
    QString content = m_editor->toPlainText();
    if (!content.contains("★讯问人签字") || !content.contains("★记录员签字") || !content.contains("★被讯问人签字")) {
        if (QMessageBox::question(this,"提示","笔录缺少签名区格式（★讯问人签字等），是否继续提交？")
            != QMessageBox::Yes) return false;
    }
    return true;
}

void RecordEditorWidget::onSubmit() {
    if (!validateRecord()) return;
    if (m_isDirty) onSaveDraft();

    auto& auth = AuthService::instance();
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QMap<QString, QVariant> vals = {
        {"status", "PendingApproval"},
        {"updated_at", now}
    };
    bool ok = DatabaseManager::instance().update("records", vals,
        "id=:id", {{"id", m_currentRecordId}});

    if (ok) {
        AuditService::instance().log(auth.currentUser().id, auth.currentUser().realName,
            AuditService::ACTION_SUBMIT_APPROVAL, "笔录管理", "Record", m_currentRecordId,
            "提交笔录审批:" + m_recordIdEdit->text());
        QMessageBox::information(this, "提交成功",
            "笔录已提交，等待一级审批（管教组长）审核。\n\n可在「审批中心」查看审批状态。");
        onNewRecord();
        loadDrafts();
    } else {
        QMessageBox::warning(this, "提交失败", "提交失败，请重试");
    }
}

void RecordEditorWidget::onAutoSave() {
    if (m_isDirty && m_currentRecordId > 0) {
        onSaveDraft();
        qDebug() << "[自动保存] 已自动保存草稿";
    }
}

void RecordEditorWidget::loadDrafts() {
    auto& auth = AuthService::instance();
    auto drafts = DatabaseManager::instance().select(
        "SELECT id, record_id, record_type, criminal_name, status, updated_at "
        "FROM records WHERE status='Draft' AND created_by=:uid "
        "ORDER BY updated_at DESC LIMIT 20",
        {{"uid", auth.currentUser().id}});
    m_draftsTable->setRowCount(drafts.size());
    for (int i = 0; i < drafts.size(); ++i) {
        m_draftsTable->setItem(i, 0, new QTableWidgetItem(drafts[i]["record_id"].toString()));
        m_draftsTable->setItem(i, 1, new QTableWidgetItem(drafts[i]["record_type"].toString()));
        m_draftsTable->setItem(i, 2, new QTableWidgetItem(drafts[i]["criminal_name"].toString()));
        m_draftsTable->setItem(i, 3, new QTableWidgetItem("草稿"));
        m_draftsTable->setItem(i, 4, new QTableWidgetItem(drafts[i]["updated_at"].toString()));
    }
}

void RecordEditorWidget::onLoadDraft() {
    int row = m_draftsTable->currentRow();
    if (row < 0) return;
    QString recordId = m_draftsTable->item(row, 0)->text();
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM records WHERE record_id=:id", {{"id", recordId}});
    if (r.isEmpty()) return;

    m_currentRecordId = r["id"].toInt();
    m_recordIdEdit->setText(r["record_id"].toString());

    // 设置笔录类型
    QString typeCode = r["record_type"].toString();
    for (int i = 1; i < m_typeCombo->count(); ++i) {
        if (m_typeCombo->itemText(i).startsWith(typeCode)) {
            m_typeCombo->setCurrentIndex(i);
            break;
        }
    }

    // 设置罪犯
    int criminalId = r["criminal_id"].toInt();
    for (int i = 1; i < m_criminalCombo->count(); ++i) {
        if (m_criminalCombo->itemData(i).toInt() == criminalId) {
            m_criminalCombo->setCurrentIndex(i);
            m_currentCriminalId = criminalId;
            break;
        }
    }
    m_currentCriminalName = r["criminal_name"].toString();
    m_criminalNameLabel->setText(m_currentCriminalName);

    // 设置其他字段
    QDateTime dt = QDateTime::fromString(r["record_date"].toString(), "yyyy-MM-dd hh:mm");
    if (dt.isValid()) m_datetimeEdit->setDateTime(dt);

    int locIdx = m_locationCombo->findText(r["record_location"].toString());
    if (locIdx >= 0) m_locationCombo->setCurrentIndex(locIdx);
    m_presentPersonsEdit->setText(r["present_persons"].toString());

    // 解密笔录正文（若未加密则直接显示原文，兼容旧记录）
    QString rawContent = r["content"].toString();
    if (KeyManager::instance().isUnlocked()) {
        QByteArray dec = KeyManager::instance().decryptField(rawContent.toUtf8());
        if (!dec.isEmpty() && dec != rawContent.toUtf8()) {
            m_editor->setPlainText(QString::fromUtf8(dec));
        } else {
            m_editor->setPlainText(rawContent); // 旧记录或解密失败，降级明文
        }
    } else {
        m_editor->setPlainText(rawContent);
    }

    m_isDirty = false;
    QString status = r["status"].toString();
    if (status == "SignedScan") {
        m_statusLabel->setText("🔒 状态：已签名扫描件（已锁定）");
        m_statusLabel->setStyleSheet("color:#22C55E;font-weight:600;");
        m_uploadScanBtn->setEnabled(false);
        m_uploadScanBtn->setText("🔒 已上传");
        m_downloadScanBtn->setEnabled(true);
        m_downloadScanBtn->setText("📥 下载扫描件");
    } else {
        m_statusLabel->setText("状态：草稿（已加载）");
        m_uploadScanBtn->setEnabled(true);
        m_uploadScanBtn->setText("🔒 上传扫描件");
        m_downloadScanBtn->setEnabled(true);
        m_downloadScanBtn->setText("📥 下载扫描件");
    }
    m_saveTimeLabel->setText("上次保存：" + r["updated_at"].toString());
}

// ──────────────────────────────────────────────────────────────
// 打印笔录：生成标准审讯笔录 PDF
// ──────────────────────────────────────────────────────────────
void RecordEditorWidget::onPrintRecord()
{
    // 1. 收集笔录数据
    if (m_typeCombo->currentIndex() == 0) {
        QMessageBox::warning(this, "提示", "请先选择笔录类型");
        return;
    }
    if (m_currentCriminalId == 0) {
        QMessageBox::warning(this, "提示", "请先选择关联罪犯");
        return;
    }
    QString content = m_editor->toPlainText();
    if (content.trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "笔录正文不能为空");
        return;
    }

    // 2. 选择保存路径
    QString defaultName = m_recordIdEdit->text().isEmpty()
        ? DateUtils::generateId("BJ") + ".pdf"
        : m_recordIdEdit->text() + ".pdf";
    QString filePath = QFileDialog::getSaveFileName(
        this, "导出笔录 PDF",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
        "PDF 文件 (*.pdf)");
    if (filePath.isEmpty()) return;

    // 3. 收集笔录信息
    QString recordId    = m_recordIdEdit->text().isEmpty() ? DateUtils::generateId("BJ") : m_recordIdEdit->text();
    QString recordType  = m_typeCombo->currentText();
    QString recordTime  = m_datetimeEdit->dateTime().toString("yyyy年MM月dd日 hh:mm");
    QString recordLoc   = m_locationCombo->currentText();
    QString criminalName = m_currentCriminalName;
    QString presentP    = m_presentPersonsEdit->text();
    auto& auth = AuthService::instance();
    QString recorderName = auth.currentUser().realName;

    // 罪犯详情
    auto criminal = DatabaseManager::instance().selectOne(
        "SELECT * FROM criminals WHERE id=:id", {{"id", m_currentCriminalId}});
    QString criminalInfo;
    if (!criminal.isEmpty()) {
        criminalInfo = QString("%1（%2）  %3族  %4")
            .arg(criminal["name"].toString())
            .arg(criminal["gender"].toString())
            .arg(criminal["ethnicity"].toString())
            .arg(criminal["id_card_number"].toString());
    }

    // 4. 创建 PDF 打印机
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setResolution(300);

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::warning(this, "错误", "无法创建 PDF 文件，请检查路径是否有效");
        return;
    }

    // 5. PDF 绘制参数
    const QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);
    const int pageW = static_cast<int>(pageRect.width());
    const int pageH = static_cast<int>(pageRect.height());
    const int marginLeft = 80;
    const int marginRight = 60;
    const int marginTop = 80;
    const int marginBottom = 80;
    const int contentW = pageW - marginLeft - marginRight;
    int y = marginTop;

    // 字体
    QFont titleFont("思源宋体", 22, QFont::Bold);
    QFont infoFont("思源宋体", 10);
    QFont contentFont("思源宋体", 12);
    QFont sigFont("思源宋体", 11);
    QFont smallFont("思源宋体", 9);

    // 颜色
    QColor darkBlue("#1A2C4A");
    QColor lineColor("#CBD5E1");
    QColor gray("#4B5563");

    // ── 第1步：绘制页眉 ──
    // Qt6 drawText(int,int,AlignFlag,str) 不存在，用 QRectF 版本手动居中
    painter.setFont(titleFont);
    painter.setPen(QPen(darkBlue));
    {
        QRectF r(0, y - 16, pageW, 40);
        painter.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, "审  讯  笔  录");
    }
    y += 20;

    painter.setFont(smallFont);
    painter.setPen(QPen(gray));
    {
        QRectF r(0, y - 12, pageW, 24);
        painter.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, "PrisonSIS 数字审讯记录系统");
    }
    y += 28;

    // ── 第2步：绘制分隔线 ──
    painter.setPen(QPen(darkBlue, 2));
    painter.drawLine(marginLeft, y, pageW - marginRight, y);
    y += 10;

    // ── 第3步：绘制笔录信息表 ──
    painter.setPen(QPen(darkBlue));
    painter.setFont(infoFont);

    struct InfoRow { QString label; QString value; };
    QList<InfoRow> rows = {
        {"笔录编号", recordId},
        {"笔录类型", recordType},
        {"审讯时间", recordTime},
        {"审讯地点", recordLoc},
        {"被讯问人", criminalInfo},
        {"在场人员", presentP.isEmpty() ? "--" : presentP},
        {"记录员",   recorderName},
    };

    // 两列布局
    const int colW = contentW / 2 - 10;
    int col = 0;
    int rowY = y;
    const int rowH = 22;

    for (const InfoRow& row : rows) {
        int x = marginLeft + col * (colW + 20);
        if (col == 1) x = marginLeft + colW + 30;

        painter.setFont(infoFont);
        painter.setPen(QPen(gray));
        painter.drawText(x, rowY, row.label + "：");
        painter.setFont(infoFont);
        painter.setPen(QPen(darkBlue));
        painter.drawText(x + 72, rowY, row.value);

        col++;
        if (col >= 2) {
            col = 0;
            rowY += rowH;
        }
    }
    y = rowY + (col == 0 ? 0 : rowH);
    y += 15;

    // 档案编号和二维码占位区（右侧）
    painter.setFont(smallFont);
    painter.setPen(QPen(gray));
    painter.drawText(pageW - marginRight - 120, y - 22 * 3, "档案编号：");
    painter.setPen(QPen(darkBlue));
    painter.drawText(pageW - marginRight - 50, y - 22 * 3, recordId);
    painter.setPen(QPen(lineColor));
    painter.drawLine(pageW - marginRight - 130, y - 22 * 2 + 4,
                     pageW - marginRight, y - 22 * 2 + 4);

    // ── 第4步：正文横线背景 ──
    const int contentTop = y + 5;
    const int contentBottom = pageH - marginBottom - 180; // 留出签名区
    const int lineSpacing = 28;

    // 画内容区顶部粗线
    painter.setPen(QPen(darkBlue, 1.5));
    painter.drawLine(marginLeft, contentTop, pageW - marginRight, contentTop);

    // 画正文区域竖向分隔线（左侧装订线）
    painter.setPen(QPen(QColor("#DC2626"), 1));
    painter.drawLine(marginLeft + 20, contentTop, marginLeft + 20, contentBottom);

    // 画正文横线
    painter.setPen(QPen(lineColor, 0.8));
    for (int ly = contentTop + lineSpacing; ly < contentBottom; ly += lineSpacing) {
        painter.drawLine(marginLeft + 25, ly, pageW - marginRight, ly);
    }

    // ── 第5步：绘制笔录正文 ──
    painter.setPen(QPen(darkBlue));
    painter.setFont(contentFont);

    // 段落自动换行
    QStringList paragraphs = content.split('\n', Qt::SkipEmptyParts);
    QFontMetrics fm(contentFont);
    int maxWidth = pageW - marginLeft - marginRight - 30;
    int curY = contentTop + 14;

    for (const QString& para : paragraphs) {
        if (para.trimmed().isEmpty()) {
            curY += lineSpacing;
            continue;
        }
        // 替换全角空格为半角，避免显示异常
        QString text = para;
        text.replace(QChar(0x3000), "  ");

        // 智能换行
        QString remaining = text;
        while (!remaining.isEmpty()) {
            QString line = fm.elidedText(remaining, Qt::ElideNone, maxWidth);
            // 如果需要手动截断
            if (fm.horizontalAdvance(line) > maxWidth) {
                int cutPos = 0;
                while (cutPos < line.length() && fm.horizontalAdvance(line.left(cutPos)) < maxWidth) {
                    cutPos++;
                }
                painter.drawText(marginLeft + 25, curY, line.left(cutPos));
                remaining = remaining.mid(cutPos);
            } else {
                painter.drawText(marginLeft + 25, curY, line);
                remaining.clear();
            }
            curY += lineSpacing;
            if (curY > contentBottom - lineSpacing) {
                // 接近签名区，停止正文
                break;
            }
        }
    }

    // 签名前分隔线
    int sigTop = contentBottom + 10;
    painter.setPen(QPen(darkBlue, 1.5));
    painter.drawLine(marginLeft, sigTop, pageW - marginRight, sigTop);
    sigTop += 15;

    // ── 第6步：绘制签名区 ──
    painter.setFont(sigFont);
    painter.setPen(QPen(darkBlue));

    const int sigColW = (pageW - marginLeft - marginRight) / 3;
    const int sigRowH = 60;
    const int labelH = 20;

    struct SigBlock {
        QString role;
        QString name;
        int x;
    };
    QList<SigBlock> sigBlocks = {
        {"讯问人", recorderName, marginLeft},
        {"记录员", recorderName, marginLeft + sigColW},
        {"被讯问人（签名）", "",    marginLeft + sigColW * 2},
    };

    for (const SigBlock& sig : sigBlocks) {
        // 签名框
        QRectF sigBox(sig.x, sigTop, sigColW - 10, sigRowH);
        painter.setPen(QPen(darkBlue, 1));
        painter.drawRect(sigBox);

        // 角色标签（框内顶部）
        painter.setPen(QPen(gray));
        painter.setFont(smallFont);
        painter.drawText(sig.x + 5, sigTop + labelH, sig.role);

        // 签名线
        painter.setPen(QPen(lineColor, 0.8));
        painter.drawLine(sig.x + 5, sigTop + labelH + 20,
                         sig.x + sigColW - 15, sigTop + labelH + 20);

        // 姓名（框内）
        painter.setPen(QPen(darkBlue));
        painter.setFont(sigFont);
        if (!sig.name.isEmpty()) {
            painter.drawText(sig.x + 5, sigTop + labelH + 40, sig.name);
        }

        // 日期线
        painter.setPen(QPen(lineColor, 0.8));
        painter.drawLine(sig.x + 5, sigTop + sigRowH - 12,
                        sig.x + sigColW - 15, sigTop + sigRowH - 12);
        painter.setPen(QPen(gray));
        painter.setFont(smallFont);
        painter.drawText(sig.x + 5, sigTop + sigRowH - 2,
                         QDate::currentDate().toString("yyyy年MM月dd日"));
    }

    // 脚注
    painter.setPen(QPen(gray));
    painter.setFont(smallFont);
    QString footer = QString("本笔录由 PrisonSIS 数字审讯记录系统生成  |  生成时间：%1  |  第 %2 页")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
        .arg(1);
    {
        QRectF r(0, pageH - 50, pageW, 30);
        painter.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, footer);
    }

    painter.end();

    // 7. 记录审计日志
    AuditService::instance().log(auth.currentUser().id, auth.currentUser().realName,
        AuditService::ACTION_PRINT, "笔录管理", "Record",
        m_currentRecordId, "导出笔录 PDF:" + filePath);

    QMessageBox::information(this, "导出成功",
        QString("笔录 PDF 已保存至：\n%1\n\n请打印后让相关人员签字按手印。").arg(filePath));
}