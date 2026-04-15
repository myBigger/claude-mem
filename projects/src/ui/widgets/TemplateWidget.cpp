#include "ui/widgets/TemplateWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "utils/DateUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QInputDialog>

TemplateWidget::TemplateWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadTemplates();
}

void TemplateWidget::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);

    // 工具栏
    QHBoxLayout* toolbar = new QHBoxLayout;
    QPushButton* newBtn = new QPushButton("新建模板");
    QPushButton* editBtn = new QPushButton("编辑");
    QPushButton* previewBtn = new QPushButton("预览");
    QPushButton* importBtn = new QPushButton("导入模板");
    QPushButton* exportBtn = new QPushButton("导出模板");
    toolbar->addWidget(newBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(previewBtn);
    toolbar->addStretch();
    toolbar->addWidget(importBtn);
    toolbar->addWidget(exportBtn);

    // 模板列表
    QGroupBox* listBox = new QGroupBox("模板列表");
    m_table = new QTableWidget;
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"启用","模板编号","模板名称","对应笔录类型","版本","操作"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setMaximumHeight(200);
    m_table->setColumnWidth(0, 50);
    m_table->setColumnWidth(1, 80);
    m_table->setColumnWidth(2, 150);
    m_table->setColumnWidth(3, 120);
    m_table->setColumnWidth(4, 60);
    QVBoxLayout* listLayout = new QVBoxLayout(listBox);
    listLayout->addWidget(m_table);

    // 预览区
    QGroupBox* previewBox = new QGroupBox("模板内容预览");
    m_preview = new QTextEdit;
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont("Microsoft YaHei", 12));
    QVBoxLayout* previewLayout = new QVBoxLayout(previewBox);
    previewLayout->addWidget(m_preview);

    main->addLayout(toolbar);
    main->addWidget(listBox, 0);
    main->addWidget(previewBox, 1);

    // 信号
    connect(newBtn, &QPushButton::clicked, this, &TemplateWidget::onNewTemplate);
    connect(editBtn, &QPushButton::clicked, this, [this]() { onEditTemplate(QModelIndex()); });
    connect(previewBtn, &QPushButton::clicked, this, &TemplateWidget::onPreview);
    connect(importBtn, &QPushButton::clicked, this, &TemplateWidget::onImport);
    connect(exportBtn, &QPushButton::clicked, this, &TemplateWidget::onExport);
    connect(m_table, &QTableWidget::itemClicked, [=](QTableWidgetItem* item) {
        if (item->column() == 0) {
            onToggleStatus(item->row(), item->column());
        } else {
            onPreview();
        }
    });
    connect(m_table, &QTableWidget::doubleClicked, this, &TemplateWidget::onEditTemplate);
}

void TemplateWidget::loadTemplates() {
    auto templates = DatabaseManager::instance().select(
        "SELECT id, template_id, name, record_type, version, content, status "
        "FROM templates ORDER BY record_type, version DESC", {});
    m_table->setRowCount(templates.size());
    for (int i = 0; i < templates.size(); ++i) {
        const auto& t = templates[i];
        QTableWidgetItem* statusItem = new QTableWidgetItem(
            t["status"].toString() == "Enabled" ? "🟢" : "🔴");
        statusItem->setData(Qt::UserRole, t["id"].toInt());
        m_table->setItem(i, 0, statusItem);
        m_table->setItem(i, 1, new QTableWidgetItem(t["template_id"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(t["name"].toString()));
        m_table->setItem(i, 3, new QTableWidgetItem(t["record_type"].toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(QString("v%1").arg(t["version"].toInt())));
        m_table->setItem(i, 5, new QTableWidgetItem(t["status"].toString() == "Enabled" ? "已启用" : "已禁用"));
    }
}

void TemplateWidget::onToggleStatus(int row, int) {
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto t = DatabaseManager::instance().selectOne(
        "SELECT status, template_id, name FROM templates WHERE id=:id", {{"id", id}});
    if (t.isEmpty()) return;
    QString newStatus = t["status"].toString() == "Enabled" ? "Disabled" : "Enabled";
    DatabaseManager::instance().update("templates",
        {{"status", newStatus}}, "id=:id", {{"id", id}});
    loadTemplates();
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_UPDATE, "模板管理", "Template", id,
        QString("模板%1状态变更为:%2").arg(t["name"].toString()).arg(newStatus));
}

void TemplateWidget::onPreview() {
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto t = DatabaseManager::instance().selectOne(
        "SELECT content FROM templates WHERE id=:id", {{"id", id}});
    if (!t.isEmpty()) m_preview->setPlainText(t["content"].toString());
}

void TemplateWidget::onNewTemplate() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "新建模板", "请输入模板名称：",
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QStringList types = {"RT-01 入监笔录","RT-02 日常谈话","RT-03 案件调查",
        "RT-04 行政处罚告知","RT-05 减刑假释评估","RT-06 出监教育","RT-07 医疗检查","RT-08 亲属会见"};
    QString selectedType = QInputDialog::getItem(this, "关联笔录类型", "请选择关联的笔录类型：", types, 0, false, &ok);
    if (!ok) return;
    QString typeCode = selectedType.split(" ").value(0, selectedType);

    // 获取该类型最大版本号
    auto maxVer = DatabaseManager::instance().selectOne(
        "SELECT MAX(version) as mv FROM templates WHERE record_type=:t", {{"t", typeCode}});
    int newVersion = maxVer.value("mv").toInt() + 1;

    QString defaultContent = QString(
        "%1 笔录模板\n\n"
        "讯问人：[___讯问人___]  记录员：[___记录员___]\n"
        "审讯时间：[___YYYY-MM-DD HH:MM___]  地点：[___地点___]\n\n"
        "★讯问人签字：[___________]  ★记录员签字：[___________]  ★被讯问人签字：[___________]\n"
    ).arg(name);

    QMap<QString, QVariant> vals = {
        {"template_id", "TM-" + QString::number(QDate::currentDate().year()) + "-" + QString::number(QDate::currentDate().month())},
        {"name", name.trimmed()},
        {"record_type", typeCode},
        {"version", newVersion},
        {"content", defaultContent},
        {"status", "Enabled"},
        {"created_by", AuthService::instance().currentUser().id},
        {"created_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")},
        {"updated_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}
    };
    int newId = DatabaseManager::instance().insert("templates", vals);
    if (newId > 0) {
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_CREATE, "模板管理", "Template", newId,
            "新建模板:" + name);
        loadTemplates();
        QMessageBox::information(this, "成功", "模板已创建，请双击编辑内容");
    }
}

void TemplateWidget::onEditTemplate(const QModelIndex&) {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this,"提示","请先选中一个模板"); return; }
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto t = DatabaseManager::instance().selectOne(
        "SELECT * FROM templates WHERE id=:id", {{"id", id}});
    if (t.isEmpty()) return;

    bool ok = false;
    QString content = QInputDialog::getMultiLineText(this, "编辑模板",
        "模板内容（支持变量占位符，如[罪犯姓名]）：\n注：★开头段落为必填段落",
        t["content"].toString(), &ok);
    if (!ok) return;

    DatabaseManager::instance().update("templates",
        {{"content", content}, {"updated_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}},
        "id=:id", {{"id", id}});
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_UPDATE, "模板管理", "Template", id,
        "编辑模板内容:" + t["name"].toString());
    loadTemplates();
    onPreview();
}

void TemplateWidget::onImport() {
    QString path = QFileDialog::getOpenFileName(this, "导入模板", "", "RBT模板文件 (*.rbt)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this,"失败","文件读取失败"); return; }
    QByteArray data = f.readAll();
    f.close();
    QString content = QString::fromUtf8(data);
    // 简单格式：name|typeCode|content
    QStringList lines = content.split("\n");
    if (lines.size() < 3) { QMessageBox::warning(this,"失败","格式错误"); return; }
    QString name = lines[0].trimmed();
    QString typeCode = lines[1].trimmed();
    QString templateContent = lines.mid(2).join("\n");
    QMap<QString, QVariant> vals = {
        {"template_id", "TM-IMP-" + DateUtils::generateId("IMP").split("-").value(2)},
        {"name", name}, {"record_type", typeCode}, {"version", 1},
        {"content", templateContent}, {"status", "Enabled"},
        {"created_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")},
        {"updated_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}
    };
    int newId = DatabaseManager::instance().insert("templates", vals);
    if (newId > 0) {
        loadTemplates();
        QMessageBox::information(this, "成功", "模板导入成功");
    }
}

void TemplateWidget::onExport() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this,"提示","请先选中一个模板"); return; }
    QString path = QFileDialog::getSaveFileName(this, "导出模板", "template.rbt", "RBT模板文件 (*.rbt)");
    if (path.isEmpty()) return;
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto t = DatabaseManager::instance().selectOne(
        "SELECT name, record_type, content FROM templates WHERE id=:id", {{"id", id}});
    if (t.isEmpty()) return;
    QString exportContent = t["name"].toString() + "\n" + t["record_type"].toString() + "\n" + t["content"].toString();
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(exportContent.toUtf8());
        f.close();
        QMessageBox::information(this, "成功", "模板已导出到：" + path);
    } else {
        QMessageBox::warning(this, "失败", "文件写入失败");
    }
}

void TemplateWidget::onDelete() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this,"提示","请先选中一个模板"); return; }
    int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    if (QMessageBox::question(this,"确认","确定删除选中的模板？此操作不可恢复。") != QMessageBox::Yes) return;
    DatabaseManager::instance().remove("templates", "id=:id", {{"id", id}});
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_DELETE, "模板管理", "Template", id, "删除模板");
    QMessageBox::information(this, "成功", "模板已删除");
    loadTemplates();
}