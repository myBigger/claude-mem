#include "ui/widgets/CriminalListWidget.h"
#include "ui/widgets/CriminalDetailWidget.h"
#include "ui/widgets/CriminalEditDialog.h"
#include "ui/widgets/CriminalFetchDialog.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QStandardItem>
#include <QAbstractItemView>
#include <QItemSelectionModel>

CriminalListWidget::CriminalListWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadData();
}
CriminalListWidget::~CriminalListWidget() {}

void CriminalListWidget::setupUI() {
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("输入姓名/罪犯编号/身份证号搜索");
    QPushButton* searchBtn = new QPushButton("查询");
    m_districtCombo = new QComboBox;
    m_districtCombo->addItems({"全部监区","一监区","二监区","三监区","四监区"});
    m_archiveCombo = new QComboBox;
    m_archiveCombo->addItems({"全部状态","在押","已封存"});
    m_addBtn = new QPushButton("新增罪犯");
    m_fetchBtn = new QPushButton("从内网获取");
    m_fetchBtn->setStyleSheet(R"(
        QPushButton{background:#0066CC;color:#FFFFFF;border:none;border-radius:4px;font-size:13px;font-weight:600;padding:6px 14px;}
        QPushButton:hover{background:#0052A3;}
        QPushButton:pressed{background:#003D82;}
        QPushButton:disabled{background:#161B22;color:#6E7681;}
    )");
    m_editBtn = new QPushButton("编辑");
    m_viewBtn = new QPushButton("查看详情");
    m_delBtn = new QPushButton("封存");
    m_exportBtn = new QPushButton("导出Excel");
    m_editBtn->setEnabled(false); m_viewBtn->setEnabled(false); m_delBtn->setEnabled(false);

    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({"罪犯编号","姓名","性别","罪名","刑期","监区","监室","等级","入狱日期","状态"});
    m_table = new QTableView(this);
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setColumnWidth(0,110); m_table->setColumnWidth(1,80); m_table->setColumnWidth(2,50);
    m_table->setColumnWidth(3,100); m_table->setColumnWidth(4,70); m_table->setColumnWidth(5,70);
    m_table->setColumnWidth(6,60); m_table->setColumnWidth(7,60); m_table->setColumnWidth(8,100);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setMinimumHeight(400);

    m_countLabel = new QLabel("共 0 条");
    m_pageLabel = new QLabel("第 1 页");
    m_prevBtn = new QPushButton("上一页");
    m_nextBtn = new QPushButton("下一页");
    m_prevBtn->setEnabled(false); m_nextBtn->setEnabled(false);

    QHBoxLayout* searchBar = new QHBoxLayout;
    searchBar->addWidget(m_searchEdit, 1);
    searchBar->addWidget(new QLabel("监区："));
    searchBar->addWidget(m_districtCombo);
    searchBar->addWidget(new QLabel("状态："));
    searchBar->addWidget(m_archiveCombo);
    searchBar->addWidget(searchBtn);

    QHBoxLayout* btnBar = new QHBoxLayout;
    btnBar->addWidget(m_addBtn);
    btnBar->addWidget(m_fetchBtn);
    btnBar->addWidget(m_editBtn);
    btnBar->addWidget(m_viewBtn); btnBar->addWidget(m_delBtn);
    btnBar->addStretch(); btnBar->addWidget(m_exportBtn);

    QHBoxLayout* pageBar = new QHBoxLayout;
    pageBar->addWidget(m_countLabel); pageBar->addStretch();
    pageBar->addWidget(m_prevBtn); pageBar->addWidget(m_pageLabel); pageBar->addWidget(m_nextBtn);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(10); main->setContentsMargins(10,10,10,10);
    main->addLayout(searchBar); main->addLayout(btnBar);
    main->addWidget(m_table, 1); main->addLayout(pageBar);

    connect(searchBtn, &QPushButton::clicked, this, &CriminalListWidget::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &CriminalListWidget::onSearch);
    connect(m_addBtn, &QPushButton::clicked, this, &CriminalListWidget::onAdd);
    connect(m_fetchBtn, &QPushButton::clicked, this, &CriminalListWidget::onFetchFromApi);
    connect(m_editBtn, &QPushButton::clicked, this, &CriminalListWidget::onEdit);
    connect(m_viewBtn, &QPushButton::clicked, this, &CriminalListWidget::onView);
    connect(m_delBtn, &QPushButton::clicked, this, &CriminalListWidget::onDelete);
    connect(m_exportBtn, &QPushButton::clicked, this, &CriminalListWidget::onExport);
    connect(m_prevBtn, &QPushButton::clicked, [=](){ if(m_page>1) loadData(m_searchEdit->text(),m_page-1); });
    connect(m_nextBtn, &QPushButton::clicked, [=](){ if(m_page<totalPages()) loadData(m_searchEdit->text(),m_page+1); });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CriminalListWidget::onSelectionChanged);
    connect(m_table, &QTableView::doubleClicked, this, &CriminalListWidget::onView);
}

void CriminalListWidget::loadData(const QString& kw, int page) {
    m_page = page;
    int offset = (page-1)*m_pageSize;
    QStringList cond; QMap<QString,QVariant> p;
    if (!kw.isEmpty()) { cond << "(name LIKE :kw OR criminal_id LIKE :kw OR id_card_number LIKE :kw)"; p["kw"] = "%"+kw+"%"; }
    QString district = m_districtCombo->currentText();
    if (district != "全部监区") { cond << "district=:d"; p["d"] = district; }
    int archIdx = m_archiveCombo->currentIndex();
    if (archIdx == 1) cond << "archived=0";
    else if (archIdx == 2) cond << "archived=1";
    QString where = cond.isEmpty() ? "" : "WHERE " + cond.join(" AND ");
    auto cntR = DatabaseManager::instance().selectOne("SELECT COUNT(*) as c FROM criminals "+where, p);
    m_total = cntR.value("c").toInt();
    QString sql = QString("SELECT id,criminal_id,name,gender,crime,sentence_years,sentence_months,district,cell,manage_level,entry_date,archived FROM criminals %1 ORDER BY created_at DESC LIMIT :l OFFSET :o").arg(where);
    p["l"] = m_pageSize; p["o"] = offset;
    auto rows = DatabaseManager::instance().select(sql, p);
    m_model->removeRows(0, m_model->rowCount());
    for (const auto& r : rows) {
        QString s = r["archived"].toInt()==1?"已封存":"在押";
        QString sent; int sy=r["sentence_years"].toInt(),sm=r["sentence_months"].toInt();
        if(sy>0&&sm>0) sent=QString("%1年%2月").arg(sy).arg(sm);
        else if(sy>0) sent=QString("%1年").arg(sy);
        else if(sm>0) sent=QString("%1月").arg(sm);
        m_model->appendRow({
            new QStandardItem(r["criminal_id"].toString()),
            new QStandardItem(r["name"].toString()),
            new QStandardItem(r["gender"].toString()),
            new QStandardItem(r["crime"].toString()),
            new QStandardItem(sent),
            new QStandardItem(r["district"].toString()),
            new QStandardItem(r["cell"].toString()),
            new QStandardItem(r["manage_level"].toString()),
            new QStandardItem(r["entry_date"].toString()),
            new QStandardItem(s)
        });
    }
    int tp = totalPages();
    m_countLabel->setText(QString("共 %1 条").arg(m_total));
    m_pageLabel->setText(QString("第 %1 / %2 页").arg(page).arg(tp));
    m_prevBtn->setEnabled(page>1); m_nextBtn->setEnabled(page<tp);
}

int CriminalListWidget::totalPages() { return m_total==0?1:(m_total+m_pageSize-1)/m_pageSize; }

void CriminalListWidget::onSearch() { loadData(m_searchEdit->text().trimmed(), 1); }

void CriminalListWidget::onAdd() {
    CriminalEditDialog dlg(this);
    if (dlg.exec()==QDialog::Accepted) {
        loadData();
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName, AuditService::ACTION_CREATE,
            "罪犯管理", "Criminal", 0, "新增罪犯档案");
    }
}

void CriminalListWidget::onFetchFromApi() {
    CriminalFetchDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        loadData();
    }
}

void CriminalListWidget::onEdit() {
    int row = m_table->currentIndex().row();
    if (row<0) return;
    CriminalEditDialog dlg(this, m_model->item(row,0)->text());
    if (dlg.exec()==QDialog::Accepted) loadData();
}

void CriminalListWidget::onView() {
    int row = m_table->currentIndex().row();
    if (row<0) return;
    CriminalDetailWidget dlg(m_model->item(row,0)->text(), this);
    dlg.setWindowTitle("罪犯详情 - " + m_model->item(row,0)->text());
    dlg.exec();
}

void CriminalListWidget::onDelete() {
    int row = m_table->currentIndex().row();
    if (row<0) return;
    QString cid = m_model->item(row,0)->text();
    if (QMessageBox::question(this,"确认","确定封存罪犯 "+cid+" 的档案？")!=QMessageBox::Yes) return;
    bool ok = DatabaseManager::instance().update("criminals",
        {{"archived",1},{"updated_at",QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}},
        "criminal_id=:id",{{"id",cid}});
    if (ok) {
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName, AuditService::ACTION_DELETE,
            "罪犯管理", "Criminal", 0, "封存:"+cid);
        QMessageBox::information(this,"成功","档案已封存"); loadData();
    } else QMessageBox::warning(this,"失败","封存失败");
}

void CriminalListWidget::onExport() {
    QString path = QFileDialog::getSaveFileName(this,"导出","罪犯档案_"+QDate::currentDate().toString("yyyyMMdd")+".xlsx","Excel(*.xlsx)");
    if (path.isEmpty()) return;
    QMessageBox::information(this,"提示","Excel导出功能开发中，敬请期待");
}

void CriminalListWidget::onSelectionChanged() {
    bool sel = !m_table->selectionModel()->selectedRows().isEmpty();
    m_viewBtn->setEnabled(sel);
    // 普通用户仅可编辑自己主治管教的罪犯；管理员可编辑全部
    bool canEdit = sel && (AuthService::instance().isAdmin() || isCurrentUserHandler());
    m_editBtn->setEnabled(canEdit);
    m_delBtn->setEnabled(sel); // 封存操作（软删除）所有用户可用
}

bool CriminalListWidget::isCurrentUserHandler() {
    int row = m_table->currentIndex().row();
    if (row < 0) return false;
    QString cid = m_model->item(row, 0)->text();
    auto r = DatabaseManager::instance().selectOne(
        "SELECT handler_id FROM criminals WHERE criminal_id=:id", {{"id", cid}});
    return r.value("handler_id").toString() == AuthService::instance().currentUser().userId;
}
