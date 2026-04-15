#include "ui/widgets/CaseListWidget.h"
#include "ui/widgets/CaseDetailWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "models/Case.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QDate>
#include <QDebug>
#include <QGroupBox>

CaseListWidget::CaseListWidget(QWidget* parent)
    : QWidget(parent)
    , m_table(nullptr)
    , m_selectedCaseId(-1)
{
    setupUi();
    loadCases();
}

CaseListWidget::~CaseListWidget() = default;

void CaseListWidget::setupUi()
{
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(10);

    // 搜索过滤栏
    QGroupBox* filterBox = new QGroupBox("筛选条件");
    QGridLayout* filterLayout = new QGridLayout(filterBox);

    // 第一行
    QLabel* searchLabel = new QLabel("关键字搜索:");
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("案号/案件名称/承办人...");
    filterLayout->addWidget(searchLabel, 0, 0);
    filterLayout->addWidget(m_searchEdit, 0, 1, 1, 2);

    QLabel* statusLabel = new QLabel("案件状态:");
    m_statusFilter = new QComboBox;
    m_statusFilter->addItems({"全部", "侦查中", "审查起诉", "已结案"});
    filterLayout->addWidget(statusLabel, 0, 3);
    filterLayout->addWidget(m_statusFilter, 0, 4);

    // 第二行
    QLabel* typeLabel = new QLabel("案件类型:");
    m_typeFilter = new QComboBox;
    m_typeFilter->addItems({"全部", "刑事案件", "行政案件", "民事案件", "执行案件", "申诉案件", "其他"});
    filterLayout->addWidget(typeLabel, 1, 0);
    filterLayout->addWidget(m_typeFilter, 1, 1);

    QLabel* dateLabel = new QLabel("立案日期:");
    m_startDateFilter = new QDateEdit;
    m_startDateFilter->setCalendarPopup(true);
    m_startDateFilter->setDate(QDate::currentDate().addMonths(-3));
    filterLayout->addWidget(dateLabel, 1, 2);
    filterLayout->addWidget(m_startDateFilter, 1, 3);

    QLabel* toLabel = new QLabel("至");
    m_endDateFilter = new QDateEdit;
    m_endDateFilter->setCalendarPopup(true);
    m_endDateFilter->setDate(QDate::currentDate());
    filterLayout->addWidget(toLabel, 1, 4);
    filterLayout->addWidget(m_endDateFilter, 1, 5);

    // 按钮行
    QHBoxLayout* btnRow = new QHBoxLayout;
    m_searchBtn = new QPushButton("🔍 搜索");
    m_newBtn = new QPushButton("➕ 新建案件");
    m_exportBtn = new QPushButton("📤 导出");
    QPushButton* refreshBtn = new QPushButton("🔄 刷新");
    m_countLabel = new QLabel("共 0 条记录");
    m_countLabel->setStyleSheet("color:#AABBCC;");

    btnRow->addWidget(m_searchBtn);
    btnRow->addWidget(m_newBtn);
    btnRow->addWidget(m_exportBtn);
    btnRow->addWidget(refreshBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_countLabel);

    QVBoxLayout* filterMain = new QVBoxLayout;
    filterMain->addLayout(filterLayout);
    filterMain->addLayout(btnRow);
    filterBox->setLayout(filterMain);
    main->addWidget(filterBox);

    // 案件列表
    QGroupBox* listBox = new QGroupBox("案件列表");
    QVBoxLayout* listLayout = new QVBoxLayout(listBox);

    m_table = new QTableWidget;
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        "ID", "案号", "案件名称", "案件类型", "状态",
        "立案日期", "承办人", "更新日期", "操作"
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->setVisible(false);

    // 设置列宽
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(1, 120);
    m_table->setColumnWidth(2, 200);
    m_table->setColumnWidth(3, 100);
    m_table->setColumnWidth(4, 80);
    m_table->setColumnWidth(5, 100);
    m_table->setColumnWidth(6, 100);
    m_table->setColumnWidth(7, 120);
    m_table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::Stretch);

    listLayout->addWidget(m_table);
    main->addWidget(listBox, 1);

    // 连接信号
    connect(m_searchBtn, &QPushButton::clicked, this, &CaseListWidget::onSearchClicked);
    connect(m_newBtn, &QPushButton::clicked, this, &CaseListWidget::onNewClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &CaseListWidget::onExportClicked);
    connect(refreshBtn, &QPushButton::clicked, this, &CaseListWidget::onRefreshClicked);
    connect(m_table, &QTableWidget::itemClicked, this, [this](QTableWidgetItem* item) {
        if (item->column() == 8) {
            // 操作列点击
            onCaseContextMenu(QCursor::pos());
        }
    });
    connect(m_table, &QTableWidget::doubleClicked, this, &CaseListWidget::onOpenCase);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &CaseListWidget::onCaseContextMenu);
}

void CaseListWidget::loadCases(const QString& filter)
{
    QString sql = "SELECT * FROM cases WHERE 1=1";
    QMap<QString, QVariant> params;

    // 关键字过滤
    if (!filter.isEmpty()) {
        sql += " AND (case_number LIKE :kw OR case_name LIKE :kw OR handler LIKE :kw)";
        params[":kw"] = "%" + filter + "%";
    }

    // 状态过滤
    int statusIdx = m_statusFilter->currentIndex();
    if (statusIdx > 0) {
        QString statusValue;
        if (statusIdx == 1) statusValue = "investigation";
        else if (statusIdx == 2) statusValue = "pending_trial";
        else if (statusIdx == 3) statusValue = "closed";
        sql += " AND status=:status";
        params[":status"] = statusValue;
    }

    // 类型过滤
    int typeIdx = m_typeFilter->currentIndex();
    if (typeIdx > 0) {
        sql += " AND case_type=:type";
        params[":type"] = Case::CASE_TYPES[typeIdx - 1];
    }

    // 日期过滤
    sql += " AND filing_date BETWEEN :startDate AND :endDate";
    params[":startDate"] = m_startDateFilter->date().toString("yyyy-MM-dd");
    params[":endDate"] = m_endDateFilter->date().toString("yyyy-MM-dd");

    sql += " ORDER BY updated_at DESC";

    auto results = DatabaseManager::instance().select(sql, params);
    m_table->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        const auto& row = results[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(row["id"].toInt())));
        m_table->setItem(i, 1, new QTableWidgetItem(row["case_number"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(row["case_name"].toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(row["case_type"].toString()));

        // 状态显示
        QString status = row["status"].toString();
        QString statusDisplay;
        if (status == "investigation") {
            statusDisplay = "🔵 侦查中";
        } else if (status == "pending_trial") {
            statusDisplay = "🟡 审查起诉";
        } else if (status == "closed") {
            statusDisplay = "🟢 已结案";
        }
        m_table->setItem(i, 4, new QTableWidgetItem(statusDisplay));

        m_table->setItem(i, 5, new QTableWidgetItem(row["filing_date"].toString()));
        m_table->setItem(i, 6, new QTableWidgetItem(row["handler"].toString()));
        m_table->setItem(i, 7, new QTableWidgetItem(row["updated_at"].toString().section(" ", 0, 0)));
        m_table->setItem(i, 8, new QTableWidgetItem("📋 操作"));

        // 存储 ID
        m_table->item(i, 0)->setData(Qt::UserRole, row["id"]);
    }

    m_countLabel->setText(QString("共 %1 条记录").arg(results.size()));
}

void CaseListWidget::onSearchClicked()
{
    loadCases(m_searchEdit->text().trimmed());
}

void CaseListWidget::onNewClicked()
{
    CaseDetailWidget* dlg = new CaseDetailWidget(-1, this);
    connect(dlg, &CaseDetailWidget::caseSaved, this, [this](const Case& c) {
        Q_UNUSED(c);
        loadCases();
        emit caseCreated(c.id);
    });
    dlg->show();
}

void CaseListWidget::onOpenCase()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int caseId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    CaseDetailWidget* dlg = new CaseDetailWidget(caseId, this);
    connect(dlg, &CaseDetailWidget::caseSaved, this, [this](const Case& c) {
        Q_UNUSED(c);
        loadCases();
        emit caseUpdated(c.id);
    });
    dlg->show();
}

void CaseListWidget::onEditCase()
{
    onOpenCase();
}

void CaseListWidget::onDeleteCase()
{
    int row = m_table->currentRow();
    if (row < 0) return;

    int caseId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    QString caseName = m_table->item(row, 2)->text();

    if (QMessageBox::warning(this, "确认删除",
        QString("确定要删除案件「%1」吗？\n\n此操作不可恢复！").arg(caseName),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 检查是否有关联笔录
    auto linked = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records WHERE criminal_id IN "
        "(SELECT id FROM suspects WHERE case_id=:caseId)",
        {{":caseId", caseId}});

    if (linked.value("cnt").toInt() > 0) {
        QMessageBox::warning(this, "无法删除",
            "该案件有关联笔录，无法直接删除！");
        return;
    }

    if (DatabaseManager::instance().remove("cases", "id=:id", {{":id", caseId}})) {
        AuditService::instance().log(
            AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_DELETE, "案件管理", "Case", caseId,
            "删除案件:" + caseName);
        QMessageBox::information(this, "成功", "案件已删除");
        loadCases();
    }
}

void CaseListWidget::onExportClicked()
{
    QString path = QFileDialog::getSaveFileName(this, "导出案件列表",
        QString("cases_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
        "CSV 文件 (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建文件！");
        return;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\uFEFF"; // BOM
    out << "案号,案件名称,案件类型,状态,立案日期,承办人,更新时间\n";

    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList line;
        for (int col = 1; col <= 7; ++col) {
            line << "\"" + m_table->item(row, col)->text().replace("\"", "\"\"") + "\"";
        }
        out << line.join(",") << "\n";
    }
    f.close();

    AuditService::instance().log(
        AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_UPDATE, "案件管理", "Case", 0,
        "导出案件列表");

    QMessageBox::information(this, "成功", QString("已导出到：\n%1").arg(path));
}

void CaseListWidget::onRefreshClicked()
{
    loadCases();
}

void CaseListWidget::onCaseContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_table->itemAt(pos);
    if (!item) return;

    int row = item->row();
    m_selectedCaseId = m_table->item(row, 0)->data(Qt::UserRole).toInt();

    QMenu menu(this);
    menu.addAction("📂 打开详情", this, &CaseListWidget::onOpenCase);
    menu.addAction("✏️ 编辑案件", this, &CaseListWidget::onEditCase);
    menu.addSeparator();
    menu.addAction("🗑️ 删除案件", this, &CaseListWidget::onDeleteCase);
    menu.exec(QCursor::pos());
}

Case CaseListWidget::getCaseFromRow(int row) const
{
    Case c;
    c.id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    c.caseNumber = m_table->item(row, 1)->text();
    c.caseName = m_table->item(row, 2)->text();
    c.caseType = m_table->item(row, 3)->text();
    c.filingDate = QDate::fromString(m_table->item(row, 5)->text(), "yyyy-MM-dd");
    c.handler = m_table->item(row, 6)->text();
    return c;
}

void CaseListWidget::refresh()
{
    loadCases();
}
