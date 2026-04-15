#include "ui/widgets/ArchiveWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "services/KeyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPageSize>
#include <QHeaderView>
#include <QDate>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QPainter>
#include <QDateTime>

ArchiveWidget::ArchiveWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadArchives();
}

void ArchiveWidget::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(8);

    // 搜索栏
    QGroupBox* searchBox = new QGroupBox("档案检索");
    QGridLayout* searchGrid = new QGridLayout(searchBox);

    searchGrid->addWidget(new QLabel("关键词："), 0, 0);
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("输入笔录编号/罪犯姓名/正文关键词...");
    searchGrid->addWidget(m_searchEdit, 0, 1);

    searchGrid->addWidget(new QLabel("笔录类型："), 0, 2);
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({"全部","RT-01 入监笔录","RT-02 日常谈话","RT-03 案件调查",
        "RT-04 行政处罚告知","RT-05 减刑假释评估","RT-06 出监教育","RT-07 医疗检查","RT-08 亲属会见"});
    searchGrid->addWidget(m_typeCombo, 0, 3);

    searchGrid->addWidget(new QLabel("开始日期："), 1, 0);
    m_startDateEdit = new QDateEdit;
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDate(QDate::currentDate().addMonths(-1));
    searchGrid->addWidget(m_startDateEdit, 1, 1);

    searchGrid->addWidget(new QLabel("结束日期："), 1, 2);
    m_endDateEdit = new QDateEdit;
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDate(QDate::currentDate());
    searchGrid->addWidget(m_endDateEdit, 1, 3);

    QHBoxLayout* btnBar = new QHBoxLayout;
    QPushButton* searchBtn = new QPushButton("搜索");
    QPushButton* statBtn = new QPushButton("统计报表");
    QPushButton* exportBtn = new QPushButton("导出PDF");
    QPushButton* batchBtn = new QPushButton("批量导出");
    btnBar->addWidget(searchBtn);
    btnBar->addWidget(statBtn);
    btnBar->addStretch();
    btnBar->addWidget(exportBtn);
    btnBar->addWidget(batchBtn);
    searchGrid->addLayout(btnBar, 2, 0, 1, 4);

    // 档案列表
    QGroupBox* listBox = new QGroupBox("已归档笔录");
    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"归档编号","罪犯","笔录类型","审讯时间",
        "审讯地点","归档人","操作"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    QVBoxLayout* listLayout = new QVBoxLayout(listBox);
    listLayout->addWidget(m_table);

    // 预览区
    QGroupBox* previewBox = new QGroupBox("档案内容预览（只读）");
    m_preview = new QTextEdit;
    m_preview->setReadOnly(true);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewBox);
    previewLayout->addWidget(m_preview);

    QHBoxLayout* contentLayout = new QHBoxLayout;
    contentLayout->addWidget(listBox, 1);
    contentLayout->addWidget(previewBox, 1);

    main->addWidget(searchBox, 0);
    main->addLayout(contentLayout, 1);

    // 信号
    connect(searchBtn, &QPushButton::clicked, this, &ArchiveWidget::onSearch);
    connect(searchBtn, &QPushButton::clicked, this, &ArchiveWidget::loadArchives);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ArchiveWidget::onSearch);
    connect(m_table, &QTableWidget::itemClicked, this, &ArchiveWidget::onPreview);
    connect(exportBtn, &QPushButton::clicked, this, &ArchiveWidget::onExportPdf);
    connect(batchBtn, &QPushButton::clicked, this, &ArchiveWidget::onBatchExport);
    connect(statBtn, &QPushButton::clicked, this, &ArchiveWidget::onStatistics);
}

void ArchiveWidget::loadArchives() {
    QStringList cond;
    QMap<QString, QVariant> params;

    QString keyword = m_searchEdit->text().trimmed();
    if (!keyword.isEmpty()) {
        cond << "(r.record_id LIKE :kw OR r.criminal_name LIKE :kw OR r.content LIKE :kw)";
        params["kw"] = "%" + keyword + "%";
    }

    int typeIdx = m_typeCombo->currentIndex();
    if (typeIdx > 0) {
        QString typeCode = QString("RT-%1").arg(typeIdx, 2, 10, QChar('0'));
        cond << "r.record_type=:t";
        params["t"] = typeCode;
    }

    QString startDate = m_startDateEdit->date().toString("yyyy-MM-dd");
    QString endDate = m_endDateEdit->date().toString("yyyy-MM-dd");
    cond << "r.record_date >= :sd";
    cond << "r.record_date <= :ed";
    params["sd"] = startDate;
    params["ed"] = endDate + " 23:59:59";

    QString where = "WHERE " + cond.join(" AND ");
    QString sql = QString(R"(
        SELECT r.id, r.record_id, r.criminal_name, r.record_type, r.record_date,
               r.record_location, r.status,
               u.real_name as archiver
        FROM records r
        LEFT JOIN users u ON r.created_by = u.id
        %1
        ORDER BY r.updated_at DESC
        LIMIT 200
    )").arg(where);

    auto archives = DatabaseManager::instance().select(sql, params);
    m_table->setRowCount(archives.size());
    for (int i = 0; i < archives.size(); ++i) {
        const auto& a = archives[i];
        m_table->setItem(i, 0, new QTableWidgetItem(a["record_id"].toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(a["criminal_name"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(a["record_type"].toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(a["record_date"].toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(a["record_location"].toString()));
        m_table->setItem(i, 5, new QTableWidgetItem(a["archiver"].toString()));
        QString status = a["status"].toString();
        m_table->setItem(i, 6, new QTableWidgetItem(
            status == "Approved" ? "已归档" : status));
        m_table->item(i, 0)->setData(Qt::UserRole, a["id"].toInt());
    }
}

void ArchiveWidget::onSearch() {
    loadArchives();
}

void ArchiveWidget::onPreview(QTableWidgetItem* item) {
    int row = item ? item->row() : -1;
    if (row < 0) return;
    int recordId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM records WHERE id=:id", {{"id", recordId}});
    if (r.isEmpty()) return;
    m_currentRecordId = recordId;
    QString preview = QString(
        "【归档编号】GA-%1\n"
        "【笔录编号】%2\n"
        "【罪犯姓名】%3\n"
        "【笔录类型】%4\n"
        "【审讯时间】%5\n"
        "【审讯地点】%6\n"
        "━━━━━━━━━━━━━━━━━━━━\n"
        "%7"
    ).arg(r["id"].toString())
     .arg(r["record_id"].toString())
     .arg(r["criminal_name"].toString())
     .arg(r["record_type"].toString())
     .arg(r["record_date"].toString())
     .arg(r["record_location"].toString())
     .arg(KeyManager::decryptContent(r["content"].toString()));
    m_preview->setPlainText(preview);
}

void ArchiveWidget::onExportPdf() {
    if (m_currentRecordId == 0) {
        QMessageBox::warning(this, "提示", "请先在表格中选中一条笔录，再点击导出");
        return;
    }
    exportSinglePdf(m_currentRecordId);
}

void ArchiveWidget::exportSinglePdf(int recordId) {
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM records WHERE id=:id", {{"id", recordId}});
    if (r.isEmpty()) return;

    QString defaultFile = QString("%1_%2_%3.pdf")
        .arg(r["record_id"].toString())
        .arg(r["criminal_name"].toString())
        .arg(r["record_date"].toString().split(" ").value(0));

    QString path = QFileDialog::getSaveFileName(this, "导出PDF",
        defaultFile, "PDF文件 (*.pdf)");
    if (path.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setCreator("PrisonRecordTool v1.0");

    QTextDocument doc;
    QString html = QString(R"PDFHTML(
        <html><head><meta charset="UTF-8"></head><body style="font-family:SimSun;font-size:14pt;">
        <div style="text-align:center;font-size:18pt;font-weight:bold;margin-bottom:20px;">
        浙江省监狱审讯笔录
        </div>
        <table style="width:100%;font-size:12pt;border-collapse:collapse;">
        <tr><td style="border:1px solid #ccc;padding:6px;width:20%;">归档编号</td>
        <td style="border:1px solid #ccc;padding:6px;">%1</td></tr>
        <tr><td style="border:1px solid #ccc;padding:6px;">笔录编号</td>
        <td style="border:1px solid #ccc;padding:6px;">%2</td></tr>
        <tr><td style="border:1px solid #ccc;padding:6px;">罪犯姓名</td>
        <td style="border:1px solid #ccc;padding:6px;">%3</td></tr>
        <tr><td style="border:1px solid #ccc;padding:6px;">笔录类型</td>
        <td style="border:1px solid #ccc;padding:6px;">%4</td></tr>
        <tr><td style="border:1px solid #ccc;padding:6px;">审讯时间</td>
        <td style="border:1px solid #ccc;padding:6px;">%5</td></tr>
        <tr><td style="border:1px solid #ccc;padding:6px;">审讯地点</td>
        <td style="border:1px solid #ccc;padding:6px;">%6</td></tr>
        </table>
        <hr/>
        <div style="margin-top:20px;white-space:pre-wrap;line-height:1.8;">%7</div>
        <div style="margin-top:40px;page-break-before:always;">
        <div style="margin-top:20px;">
        <table style="width:100%;">
        <tr><td style="padding:8px;">讯问人签字：__________________</td>
        <td style="padding:8px;">记录员签字：__________________</td></tr>
        <tr><td style="padding:8px;">被讯问人签字：__________________</td>
        <td style="padding:8px;">日期：__________________</td></tr>
        </table>
        </div>
        </div>
        </body></html>
    )PDFHTML").arg("GA-" + r["id"].toString())
     .arg(r["record_id"].toString())
     .arg(r["criminal_name"].toString())
     .arg(r["record_type"].toString())
     .arg(r["record_date"].toString())
     .arg(r["record_location"].toString())
     .arg(KeyManager::decryptContent(r["content"].toString()).toHtmlEscaped());

    doc.setHtml(html);
    doc.print(&printer);

    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_EXPORT, "档案中心", "Record", recordId,
        "导出PDF:" + r["record_id"].toString());

    QMessageBox::information(this, "导出成功", "PDF已导出到：\n" + path);
}

void ArchiveWidget::onBatchExport() {
    if (m_table->rowCount() == 0) {
        QMessageBox::warning(this, "提示", "没有可导出的档案");
        return;
    }
    QString dir = QFileDialog::getExistingDirectory(this, "选择导出目录");
    if (dir.isEmpty()) return;

    int count = 0;
    for (int i = 0; i < m_table->rowCount() && i < 50; ++i) {
        int id = m_table->item(i, 0)->data(Qt::UserRole).toInt();
        if (id > 0) {
            auto r = DatabaseManager::instance().selectOne(
                "SELECT * FROM records WHERE id=:id", {{"id", id}});
            if (!r.isEmpty()) {
                QString fileName = QString("%1/%2_%3_%4.pdf")
                    .arg(dir)
                    .arg(r["record_id"].toString())
                    .arg(r["criminal_name"].toString())
                    .arg(r["record_date"].toString().split(" ").value(0));
                QPrinter printer(QPrinter::HighResolution);
                printer.setOutputFormat(QPrinter::PdfFormat);
                printer.setOutputFileName(fileName);
                printer.setPageSize(QPageSize(QPageSize::A4));
                QTextDocument doc;
                doc.setHtml(QString("<pre>%1</pre>").arg(r["content"].toString().toHtmlEscaped()));
                doc.print(&printer);
                count++;
            }
        }
    }
    QMessageBox::information(this, "批量导出完成", QString("成功导出 %1 份PDF到：\n%2").arg(count).arg(dir));
}

void ArchiveWidget::onStatistics() {
    auto records = DatabaseManager::instance().select(
        "SELECT record_type, COUNT(*) as cnt FROM records GROUP BY record_type", {});
    QString stats = "笔录统计报表（全部档案）\n━━━━━━━━━━━━━━━━\n";
    int total = 0;
    for (const auto& r : records) {
        stats += QString("%1 : %2 份\n").arg(r["record_type"].toString()).arg(r["cnt"].toInt());
        total += r["cnt"].toInt();
    }
    stats += QString("━━━━━━━━━━━━━━━━\n合计：%1 份").arg(total);
    m_preview->setPlainText(stats);
}