#include "ui/widgets/CriminalDetailWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/KeyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QBrush>

CriminalDetailWidget::CriminalDetailWidget(const QString& cid, QWidget* parent)
    : QDialog(parent), m_cid(cid) {
    setWindowTitle("罪犯详情 - " + cid);
    setMinimumSize(1100, 700);
    resize(1200, 750);
    setStyleSheet(R"(
        QDialog{background:#0A0E14;border:1px solid #21262D;border-radius:10px;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QGroupBox{font-weight:bold;font-size:13px;color:#0066CC;border:1px solid #21262D;border-radius:8px;margin-top:10px;background:#0F1419;padding-top:10px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 8px;background:#0F1419;}
        QPushButton{background:#1C2128;color:#E6EDF3;border:none;border-radius:4px;padding:8px 16px;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QPushButton:hover{background:#30363D;}
    )");
    loadData();
}
CriminalDetailWidget::~CriminalDetailWidget() {}

QLabel* mkL(const QString& t, bool b=false) {
    QLabel* l = new QLabel(t.isEmpty()?"--":t);
    l->setStyleSheet(QString("font-size:13px;color:#E6EDF3;") + (b?"font-weight:bold;":""));
    return l;
}

void CriminalDetailWidget::loadData() {
    auto r = DatabaseManager::instance().selectOne(
        "SELECT c.*, u.real_name as handler_name "
        "FROM criminals c LEFT JOIN users u ON c.handler_id=u.user_id "
        "WHERE c.criminal_id=:id", {{"id", m_cid}});
    if (r.isEmpty()) {
        QVBoxLayout* m = new QVBoxLayout(this);
        m->addWidget(new QLabel("未找到该罪犯信息"));
        return;
    }
    m_criminalDbId = r["id"].toInt();
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(8);
    QLabel* title = new QLabel("罪犯档案详情");
    title->setStyleSheet("font-size:22px;font-weight:600;color:#FFFFFF;padding:4px;letter-spacing:-0.3px;");
    main->addWidget(title);
    QHBoxLayout* infoRow = new QHBoxLayout;

    QGroupBox* leftBox = new QGroupBox("基本信息");
    QFormLayout* leftForm = new QFormLayout;
    leftForm->setLabelAlignment(Qt::AlignRight);
    leftForm->addRow("罪犯编号：", mkL(r["criminal_id"].toString()));
    leftForm->addRow("姓名：", mkL(r["name"].toString(), true));
    leftForm->addRow("性别：", mkL(r["gender"].toString()));
    leftForm->addRow("民族：", mkL(r["ethnicity"].toString()));
    leftForm->addRow("出生日期：", mkL(r["birth_date"].toString()));
    leftForm->addRow("身份证号：", mkL(r["id_card_number"].toString()));
    leftForm->addRow("籍贯：", mkL(r["native_place"].toString()));
    leftForm->addRow("文化程度：", mkL(r["education"].toString()));
    leftForm->addRow("备注：", mkL(r["remark"].toString()));
    leftBox->setLayout(leftForm);
    leftBox->setMaximumWidth(340);
    infoRow->addWidget(leftBox);

    QGroupBox* rightBox = new QGroupBox("案件与服刑信息");
    QFormLayout* rightForm = new QFormLayout;
    rightForm->setLabelAlignment(Qt::AlignRight);
    int sy=r["sentence_years"].toInt(), sm=r["sentence_months"].toInt();
    QString sent;
    if (sy>0 && sm>0) sent=QString("%1年%2月").arg(sy).arg(sm);
    else if (sy>0) sent=QString("%1年").arg(sy);
    else if (sm>0) sent=QString("%1月").arg(sm);
    else sent="无";
    rightForm->addRow("罪名：", mkL(r["crime"].toString(), true));
    rightForm->addRow("刑期：", mkL(sent));
    rightForm->addRow("原判法院：", mkL(r["original_court"].toString()));
    rightForm->addRow("入狱日期：", mkL(r["entry_date"].toString()));
    rightForm->addRow("监区/监室：", mkL(QString("%1/%2").arg(r["district"].toString()).arg(r["cell"].toString())));
    rightForm->addRow("管理等级：", mkL(r["manage_level"].toString()));
    rightForm->addRow("涉案类型：", mkL(r["crime_type"].toString()));
    rightForm->addRow("主治管教：", mkL(r["handler_name"].toString()));
    {
        QLabel* l = new QLabel(r["archived"].toInt()==1?"已封存":"在押");
        l->setStyleSheet(QString("font-size:13px;color:%1;").arg(r["archived"].toInt()==1?"#E53E3E":"#22C55E"));
        rightForm->addRow("档案状态：", l);
    }
    rightBox->setLayout(rightForm);
    infoRow->addWidget(rightBox, 1);
    main->addLayout(infoRow);

    QGroupBox* recBox = new QGroupBox("关联笔录");
    QVBoxLayout* recLayout = new QVBoxLayout(recBox);
    m_recordsTable = new QTableWidget;
    m_recordsTable->setColumnCount(7);
    m_recordsTable->setHorizontalHeaderLabels({"笔录编号","笔录类型","审讯时间","审讯地点","记录员","状态","操作"});
    m_recordsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recordsTable->verticalHeader()->setVisible(false);
    m_recordsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recordsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordsTable->setAlternatingRowColors(true);
    m_recordsTable->setMinimumHeight(260);
    m_recordRowToId.clear();
    auto recs = DatabaseManager::instance().select(
        "SELECT r.id,r.record_id,r.record_type,r.record_date,"
        "r.record_location,r.status,u.real_name as recorder_name "
        "FROM records r LEFT JOIN users u ON r.created_by=u.id "
        "WHERE r.criminal_id=:cid ORDER BY r.created_at DESC LIMIT 50",
        {{"cid", m_criminalDbId}});
    m_recordsTable->setRowCount(recs.size());
    for (int i=0; i<recs.size(); ++i) {
        const auto& rec = recs[i];
        m_recordRowToId[i] = rec["id"].toInt();
        m_recordsTable->setItem(i,0,new QTableWidgetItem(rec["record_id"].toString()));
        m_recordsTable->setItem(i,1,new QTableWidgetItem(rec["record_type"].toString()));
        m_recordsTable->setItem(i,2,new QTableWidgetItem(rec["record_date"].toString()));
        m_recordsTable->setItem(i,3,new QTableWidgetItem(rec["record_location"].toString()));
        m_recordsTable->setItem(i,4,new QTableWidgetItem(rec["recorder_name"].toString()));
        QString st = rec["status"].toString();
        QTableWidgetItem* si = new QTableWidgetItem(st);
        if (st=="Approved") si->setForeground(QBrush(QColor("#22C55E")));
        else if (st=="Rejected") si->setForeground(QBrush(QColor("#E53E3E")));
        else if (st=="InApproval") si->setForeground(QBrush(QColor("#3B82F6")));
        else if (st=="PendingApproval") si->setForeground(QBrush(QColor("#F59E0B")));
        m_recordsTable->setItem(i,5,si);
        m_recordsTable->setItem(i,6,new QTableWidgetItem("查看详情"));
        m_recordsTable->item(i,6)->setForeground(QBrush(QColor("#0066CC")));
    }
    recLayout->addWidget(m_recordsTable);
    main->addWidget(recBox, 1);
    QHBoxLayout* btnBar = new QHBoxLayout;
    QPushButton* newBtn = new QPushButton("新建笔录");
    QPushButton* refreshBtn = new QPushButton("刷新");
    QPushButton* closeBtn = new QPushButton("关闭");
    newBtn->setStyleSheet("QPushButton{background:#0066CC;color:#FFFFFF;padding:8px 16px;border:none;border-radius:4px;font-weight:600;}"
        "QPushButton:hover{background:#0052A3;}");
    closeBtn->setStyleSheet("QPushButton{background:#1C2128;color:#E6EDF3;padding:8px 16px;border:none;border-radius:4px;}"
        "QPushButton:hover{background:#30363D;}");
    btnBar->addWidget(newBtn);
    btnBar->addWidget(refreshBtn);
    btnBar->addStretch();
    btnBar->addWidget(closeBtn);
    main->addLayout(btnBar);
    connect(newBtn, &QPushButton::clicked, this, &CriminalDetailWidget::onNewRecord);
    connect(refreshBtn, &QPushButton::clicked, this, &CriminalDetailWidget::onRefreshRecords);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_recordsTable, &QTableWidget::cellDoubleClicked, this, &CriminalDetailWidget::onViewRecordDetail);
}

void CriminalDetailWidget::onNewRecord() {
    QMessageBox::information(this,"提示","将新建笔录，当前罪犯："+m_cid);
}
void CriminalDetailWidget::onPrintRecord() {
    QMessageBox::information(this,"提示","打印功能开发中");
}
void CriminalDetailWidget::onRefreshRecords() {
    for (int i=m_recordsTable->rowCount()-1; i>=0; --i) m_recordsTable->removeRow(i);
    m_recordRowToId.clear();
    auto recs = DatabaseManager::instance().select(
        "SELECT r.id,r.record_id,r.record_type,r.record_date,"
        "r.record_location,r.status,u.real_name as recorder_name "
        "FROM records r LEFT JOIN users u ON r.created_by=u.id "
        "WHERE r.criminal_id=:cid ORDER BY r.created_at DESC LIMIT 50",
        {{"cid", m_criminalDbId}});
    m_recordsTable->setRowCount(recs.size());
    for (int i=0; i<recs.size(); ++i) {
        const auto& rec = recs[i];
        m_recordRowToId[i] = rec["id"].toInt();
        m_recordsTable->setItem(i,0,new QTableWidgetItem(rec["record_id"].toString()));
        m_recordsTable->setItem(i,1,new QTableWidgetItem(rec["record_type"].toString()));
        m_recordsTable->setItem(i,2,new QTableWidgetItem(rec["record_date"].toString()));
        m_recordsTable->setItem(i,3,new QTableWidgetItem(rec["record_location"].toString()));
        m_recordsTable->setItem(i,4,new QTableWidgetItem(rec["recorder_name"].toString()));
        QString st = rec["status"].toString();
        QTableWidgetItem* si = new QTableWidgetItem(st);
        if (st=="Approved") si->setForeground(QBrush(QColor("#22C55E")));
        else if (st=="Rejected") si->setForeground(QBrush(QColor("#E53E3E")));
        else if (st=="InApproval") si->setForeground(QBrush(QColor("#3B82F6")));
        else if (st=="PendingApproval") si->setForeground(QBrush(QColor("#F59E0B")));
        m_recordsTable->setItem(i,5,si);
        m_recordsTable->setItem(i,6,new QTableWidgetItem("查看详情"));
        m_recordsTable->item(i,6)->setForeground(QBrush(QColor("#0066CC")));
    }
}
void CriminalDetailWidget::onViewRecordDetail(int row, int) {
    if (row < 0) return;
    int rid = m_recordRowToId.value(row, 0);
    if (rid == 0) return;
    auto rec = DatabaseManager::instance().selectOne(
        "SELECT r.*, u.real_name as recorder_name "
        "FROM records r LEFT JOIN users u ON r.created_by=u.id "
        "WHERE r.id=:id", {{"id", rid}});
    if (rec.isEmpty()) return;
    QDialog dlg(this);
    dlg.setWindowTitle("笔录详情 - " + rec["record_id"].toString());
    dlg.resize(700, 550);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    QTextEdit* te = new QTextEdit;
    te->setReadOnly(true);
    te->setHtml(QString(
        "<h2 style='text-align:center;color:#0066CC;'>%1</h2>"
        "<table width='100%' style='font-size:13px;'>"
        "<tr><td><b>笔录编号：</b>%2</td><td><b>类型：</b>%3</td></tr>"
        "<tr><td><b>审讯时间：</b>%4</td><td><b>审讯地点：</b>%5</td></tr>"
        "<tr><td><b>记录员：</b>%6</td><td><b>状态：</b>%7</td></tr>"
        "</table><hr/><pre style='font-size:13px;white-space:pre-wrap;line-height:1.8;'>%8</pre>"
    ).arg(rec["record_type"].toString()).arg(rec["record_id"].toString())
     .arg(rec["record_type"].toString()).arg(rec["record_date"].toString())
     .arg(rec["record_location"].toString()).arg(rec["recorder_name"].toString())
     .arg(rec["status"].toString()).arg(KeyManager::decryptContent(rec["content"].toString()).toHtmlEscaped()));
    v->addWidget(te);
    QPushButton* cb = new QPushButton("关闭");
    cb->setStyleSheet("QPushButton{padding:6px 24px;background:#0066CC;color:#FFFFFF;border:none;border-radius:4px;font-weight:600;}"
        "QPushButton:hover{background:#6E7AE2;}");
    connect(cb, &QPushButton::clicked, &dlg, &QDialog::accept);
    v->addWidget(cb, 0, Qt::AlignRight);
    dlg.exec();
}
