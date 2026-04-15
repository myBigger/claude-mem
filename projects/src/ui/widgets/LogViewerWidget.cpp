#include "ui/widgets/LogViewerWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QDateEdit>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDialog>
#include <QTextEdit>
#include <QDate>
#include <QBrush>
#include <QGroupBox>

LogViewerWidget::LogViewerWidget(QWidget* p) : QWidget(p) {
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(8);

    QGroupBox* queryBox = new QGroupBox("查询条件");
    QFormLayout* queryForm = new QFormLayout;
    queryForm->setLabelAlignment(Qt::AlignRight);
    m_userEdit = new QLineEdit;
    m_keywordEdit = new QLineEdit;
    m_actionCombo = new QComboBox;
    m_actionCombo->addItems({"全部", "登录", "登出", "创建", "更新", "删除", "审批通过", "审批驳回", "导出", "备份", "还原"});
    m_moduleCombo = new QComboBox;
    m_moduleCombo->addItems({"全部", "认证", "罪犯管理", "笔录制作", "审批中心", "档案中心", "模板管理", "用户管理", "数据备份"});
    m_startDate = new QDateEdit;
    m_startDate->setDate(QDate::currentDate().addDays(-30));
    m_startDate->setCalendarPopup(true);
    m_endDate = new QDateEdit;
    m_endDate->setDate(QDate::currentDate());
    m_endDate->setCalendarPopup(true);
    queryForm->addRow("操作人：", m_userEdit);
    queryForm->addRow("操作类型：", m_actionCombo);
    queryForm->addRow("模块：", m_moduleCombo);
    queryForm->addRow("关键词：", m_keywordEdit);
    queryForm->addRow("开始日期：", m_startDate);
    queryForm->addRow("结束日期：", m_endDate);
    queryBox->setLayout(queryForm);

    QHBoxLayout* btnBar = new QHBoxLayout;
    QPushButton* searchBtn = new QPushButton("查询");
    QPushButton* exportBtn = new QPushButton("导出CSV");
    QPushButton* clearBtn = new QPushButton("清空日志");
    QPushButton* refreshBtn = new QPushButton("刷新");
    searchBtn->setStyleSheet("QPushButton{background:#0066CC;color:#FFFFFF;padding:6px 20px;border:none;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#0052A3;}");
    clearBtn->setStyleSheet("QPushButton{background:#E53E3E;color:#FFFFFF;padding:6px 20px;border:none;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#C53030;}");
    btnBar->addWidget(searchBtn);
    btnBar->addWidget(exportBtn);
    btnBar->addWidget(clearBtn);
    btnBar->addWidget(refreshBtn);
    btnBar->addStretch();
    m_pageLabel = new QLabel("第 1 页 / 共 1 页");
    btnBar->addWidget(m_pageLabel);

    QHBoxLayout* topRow = new QHBoxLayout;
    topRow->addWidget(queryBox, 1);
    topRow->addLayout(btnBar);
    main->addLayout(topRow);

    m_table = new QTableWidget;
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"ID","时间","操作人","操作类型","模块","详情","IP地址"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setMinimumHeight(400);
    m_table->setColumnHidden(0, true);
    main->addWidget(m_table, 1);

    QHBoxLayout* pageBar = new QHBoxLayout;
    QPushButton* prevBtn = new QPushButton("上一页");
    QPushButton* nextBtn = new QPushButton("下一页");
    pageBar->addWidget(prevBtn);
    pageBar->addWidget(nextBtn);
    pageBar->addStretch();
    main->addLayout(pageBar);

    connect(searchBtn, &QPushButton::clicked, this, [this](){ loadLogs(1); });
    connect(refreshBtn, &QPushButton::clicked, this, [this](){ loadLogs(m_currentPage); });
    connect(exportBtn, &QPushButton::clicked, this, &LogViewerWidget::onExportCsv);
    connect(clearBtn, &QPushButton::clicked, this, &LogViewerWidget::onClearLogs);
    connect(prevBtn, &QPushButton::clicked, this, [this](){ if(m_currentPage>1) loadLogs(m_currentPage-1); });
    connect(nextBtn, &QPushButton::clicked, this, [this](){ loadLogs(m_currentPage+1); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int){ onRowDoubleClicked(row, 0); });

    loadLogs(1);
}

void LogViewerWidget::loadLogs(int page) {
    m_currentPage = page;
    QStringList cond; QMap<QString, QVariant> p;
    if (!m_userEdit->text().trimmed().isEmpty()) {
        cond << "user_name LIKE :u"; p["u"] = "%" + m_userEdit->text().trimmed() + "%";
    }
    QString action = m_actionCombo->currentText();
    if (action != "全部") {
        QMap<QString,QString> actionMap = {
            {"登录","登录"},{"登出","登出"},{"创建","创建"},{"更新","更新"},
            {"删除","删除"},{"审批通过","审批通过"},{"审批驳回","审批驳回"},
            {"导出","导出"},{"备份","备份"},{"还原","还原"}
        };
        if (actionMap.contains(action)) { cond << "action=:a"; p["a"] = actionMap[action]; }
    }
    QString module = m_moduleCombo->currentText();
    if (module != "全部") { cond << "module=:m"; p["m"] = module; }
    if (!m_keywordEdit->text().trimmed().isEmpty()) {
        cond << "(details LIKE :kw OR object_type LIKE :kw)"; p["kw"] = "%" + m_keywordEdit->text().trimmed() + "%";
    }
    QString startStr = m_startDate->date().toString("yyyy-MM-dd");
    QString endStr = m_endDate->date().addDays(1).toString("yyyy-MM-dd");
    cond << "created_at >= :sd AND created_at < :ed";
    p["sd"] = startStr; p["ed"] = endStr;
    QString where = cond.isEmpty() ? "" : "WHERE " + cond.join(" AND ");
    auto cntR = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as c FROM audit_logs " + where, p);
    m_totalCount = cntR.value("c").toInt();
    int tp = m_totalCount == 0 ? 1 : (m_totalCount + m_pageSize - 1) / m_pageSize;
    if (m_currentPage > tp) m_currentPage = tp;
    int offset = (m_currentPage - 1) * m_pageSize;
    QString sql = QString("SELECT * FROM audit_logs %1 ORDER BY created_at DESC LIMIT %2 OFFSET %3").arg(where).arg(m_pageSize).arg(offset);
    auto rows = DatabaseManager::instance().select(sql, p);
    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];
        m_table->setItem(i, 0, new QTableWidgetItem(r["id"].toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(r["created_at"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(r["user_name"].toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(r["action"].toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(r["module"].toString()));
        m_table->setItem(i, 5, new QTableWidgetItem(r["details"].toString()));
        m_table->setItem(i, 6, new QTableWidgetItem(r["ip_address"].toString()));
    }
    tp = m_totalCount == 0 ? 1 : (m_totalCount + m_pageSize - 1) / m_pageSize;
    m_pageLabel->setText(QString("第 %1 页 / 共 %2 页 (共 %3 条)").arg(m_currentPage).arg(tp).arg(m_totalCount));
}

void LogViewerWidget::onSearch() { loadLogs(1); }
void LogViewerWidget::onRefresh() { loadLogs(m_currentPage); }

void LogViewerWidget::onExportCsv() {
    if (m_table->rowCount() == 0) {
        QMessageBox::warning(this, "提示", "没有可导出的日志数据");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "导出CSV",
        "audit_logs_" + QDate::currentDate().toString("yyyyMMdd") + ".csv",
        "CSV文件(*.csv)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "失败", "文件无法写入");
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";
    out << "时间,操作人,操作类型,模块,详情,IP地址\n";
    for (int i = 0; i < m_table->rowCount(); ++i) {
        for (int j = 1; j < m_table->columnCount(); ++j) {
            out << "\"" << m_table->item(i,j)->text().replace("\"","\"\"") << "\"";
            if (j < m_table->columnCount()-1) out << ",";
        }
        out << "\n";
    }
    f.close();
    QMessageBox::information(this, "成功", "已导出到：\n" + path);
}

void LogViewerWidget::onClearLogs() {
    if (!AuthService::instance().isAdmin()) {
        QMessageBox::warning(this, "权限不足", "只有管理员可以清空日志");
        return;
    }
    if (QMessageBox::warning(this, "确认清空",
        "确定清空所有审计日志？此操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    DatabaseManager::instance().remove("audit_logs", "1=1", {});
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_DELETE, "日志审计", "AuditLog", 0, "清空所有审计日志");
    QMessageBox::information(this, "成功", "审计日志已清空");
    loadLogs(1);
}

void LogViewerWidget::onRowDoubleClicked(int row, int) {
    if (row < 0) return;
    int id = m_table->item(row, 0)->text().toInt();
    auto r = DatabaseManager::instance().selectOne(
        "SELECT * FROM audit_logs WHERE id=:id", {{"id", id}});
    if (r.isEmpty()) return;
    QDialog dlg(this);
    dlg.setWindowTitle("日志详情 - ID:" + QString::number(id));
    dlg.resize(600, 400);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    QTextEdit* te = new QTextEdit;
    te->setReadOnly(true);
    QString html = QString(
        "<h3 style='color:#0066CC;'>审计日志详情</h3>"
        "<table width='100%' style='font-size:13px;'>"
        "<tr><td style='padding:4px;'><b>日志ID：</b>%1</td>"
        "<td style='padding:4px;'><b>操作人：</b>%2</td></tr>"
        "<tr><td style='padding:4px;'><b>操作类型：</b>%3</td>"
        "<td style='padding:4px;'><b>模块：</b>%4</td></tr>"
        "<tr><td style='padding:4px;'><b>时间：</b>%5</td>"
        "<td style='padding:4px;'><b>IP：</b>%6</td></tr>"
        "<tr><td style='padding:4px;' colspan='2'><b>详情：</b>%7</td></tr>"
        "</table>"
    ).arg(r["id"].toString()).arg(r["user_name"].toString())
     .arg(r["action"].toString()).arg(r["module"].toString())
     .arg(r["created_at"].toString()).arg(r["ip_address"].toString())
     .arg(r["details"].toString().toHtmlEscaped());
    te->setHtml(html);
    v->addWidget(te);
    QPushButton* cb = new QPushButton("关闭");
    cb->setStyleSheet("QPushButton{padding:6px 24px;background:#0066CC;color:#FFF;border-radius:4px;font-weight:600;}"
        "QPushButton:hover{background:#0052A3;}");
    connect(cb, &QPushButton::clicked, &dlg, &QDialog::accept);
    v->addWidget(cb, 0, Qt::AlignRight);
    dlg.exec();
}
