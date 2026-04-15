#include "ui/widgets/UserManageWidget.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QDateTime>
#include <QAbstractItemView>
#include <QLineEdit>

UserManageWidget::UserManageWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    loadUsers();
}

void UserManageWidget::setupUI() {
    QVBoxLayout* main = new QVBoxLayout(this);

    QHBoxLayout* toolbar = new QHBoxLayout;
    QPushButton* addBtn = new QPushButton("新增用户");
    QPushButton* editBtn = new QPushButton("编辑");
    QPushButton* resetBtn = new QPushButton("密码重置");
    QPushButton* toggleBtn = new QPushButton("启用/禁用");
    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(resetBtn);
    toolbar->addWidget(toggleBtn);
    toolbar->addStretch();

    m_table = new QTableWidget;
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({"工号","姓名","角色","监区","职务","联系方式","账号状态","最后登录"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);

    main->addLayout(toolbar);
    main->addWidget(m_table, 1);

    connect(addBtn, &QPushButton::clicked, this, &UserManageWidget::onAdd);
    connect(editBtn, &QPushButton::clicked, this, &UserManageWidget::onEdit);
    connect(resetBtn, &QPushButton::clicked, this, &UserManageWidget::onResetPassword);
    connect(toggleBtn, &QPushButton::clicked, this, &UserManageWidget::onToggleEnabled);
}

void UserManageWidget::loadUsers() {
    auto users = DatabaseManager::instance().select(
        "SELECT id, user_id, username, real_name, role, department, position, phone, "
        "enabled, last_login_time FROM users ORDER BY user_id", {});
    m_table->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        const auto& u = users[i];
        m_table->setItem(i, 0, new QTableWidgetItem(u["user_id"].toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(u["real_name"].toString()));
        m_table->setItem(i, 2, new QTableWidgetItem(u["role"].toString() == "Admin" ? "管理员" : "普通用户"));
        m_table->setItem(i, 3, new QTableWidgetItem(u["department"].toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(u["position"].toString()));
        m_table->setItem(i, 5, new QTableWidgetItem(u["phone"].toString()));
        QString enabled = u["enabled"].toInt() == 1 ? "🟢 启用" : "🔴 禁用";
        m_table->setItem(i, 6, new QTableWidgetItem(enabled));
        m_table->setItem(i, 7, new QTableWidgetItem(u["last_login_time"].toString()));
        m_table->item(i, 0)->setData(Qt::UserRole, u["id"].toInt());
    }
}

void UserManageWidget::onAdd() {
    // 生成新工号
    auto maxId = DatabaseManager::instance().selectOne(
        "SELECT MAX(CAST(SUBSTR(user_id,4) AS INTEGER)) as mx FROM users", {});
    int nextNum = maxId.value("mx").toInt() + 1;
    QString newUserId = QString("US-%1").arg(nextNum, 4, 10, QChar('0'));

    bool ok = false;
    QString username = QInputDialog::getText(this, "新增用户", "登录账号（工号）：", QLineEdit::Normal, QString(), &ok);
    if (!ok || username.trimmed().isEmpty()) return;
    QString realName = QInputDialog::getText(this, "新增用户", "真实姓名：", QLineEdit::Normal, QString(), &ok);
    if (!ok || realName.trimmed().isEmpty()) return;
    QStringList roleList = {"普通用户", "管理员"};
    QString role = QInputDialog::getItem(this, "新增用户", "用户角色：", roleList, 0, true, &ok);
    if (!ok) return;
    QString dept = QInputDialog::getText(this, "新增用户", "所属监区：", QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    QMap<QString, QVariant> vals = {
        {"user_id", newUserId},
        {"username", username.trimmed()},
        {"password_hash", "098f6bcd4621d373cade4e832627b4f6"}, // 默认密码：test
        {"real_name", realName.trimmed()},
        {"role", role == "管理员" ? "Admin" : "ReadOnly"},
        {"department", dept},
        {"position", ""},
        {"enabled", 1},
        {"created_at", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}
    };
    int newId = DatabaseManager::instance().insert("users", vals);
    if (newId > 0) {
        AuditService::instance().log(AuthService::instance().currentUser().id,
            AuthService::instance().currentUser().realName,
            AuditService::ACTION_CREATE, "用户管理", "User", newId,
            "新增用户:" + username);
        loadUsers();
        QMessageBox::information(this, "成功", "用户已创建\n默认密码：test\n请提醒用户尽快修改密码");
    } else {
        QMessageBox::warning(this, "失败", "创建失败，可能用户名已存在");
    }
}

void UserManageWidget::onEdit() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this,"提示","请先选中一个用户"); return; }
    int userId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto u = DatabaseManager::instance().selectOne(
        "SELECT * FROM users WHERE id=:id", {{"id", userId}});
    if (u.isEmpty()) return;

    bool ok = false;
    QString dept = QInputDialog::getText(this, "编辑用户",
        "所属监区：", QLineEdit::Normal, u["department"].toString(), &ok);
    if (!ok) return;
    QString position = QInputDialog::getText(this, "编辑用户",
        "职务：", QLineEdit::Normal, u["position"].toString(), &ok);
    if (!ok) return;

    DatabaseManager::instance().update("users",
        {{"department", dept}, {"position", position}},
        "id=:id", {{"id", userId}});
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_UPDATE, "用户管理", "User", userId,
        "编辑用户:" + u["username"].toString());
    loadUsers();
}

void UserManageWidget::onResetPassword() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this,"提示","请先选中一个用户"); return; }
    int userId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    QString username = m_table->item(row, 0)->text();
    if (QMessageBox::question(this, "确认",
        QString("确定重置用户 %1 的密码？\n重置后密码将恢复为：test").arg(username))
        != QMessageBox::Yes) return;
    AuthService::instance().resetPassword(userId);
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_UPDATE, "用户管理", "User", userId,
        "重置密码:" + username);
    QMessageBox::information(this, "成功", "密码已重置为：test");
}

void UserManageWidget::onToggleEnabled() {
    int row = m_table->currentRow();
    if (row < 0) { QMessageBox::warning(this,"提示","请先选中一个用户"); return; }
    int userId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto u = DatabaseManager::instance().selectOne(
        "SELECT username, enabled FROM users WHERE id=:id", {{"id", userId}});
    if (u.isEmpty()) return;
    bool newEnabled = u["enabled"].toInt() == 1 ? false : true;
    DatabaseManager::instance().update("users",
        {{"enabled", newEnabled ? 1 : 0}},
        "id=:id", {{"id", userId}});
    AuditService::instance().log(AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_UPDATE, "用户管理", "User", userId,
        QString("账号%1:%2").arg(u["username"].toString())
                              .arg(newEnabled ? "启用" : "禁用"));
    loadUsers();
}