#include "ui/LoginDialog.h"
#include "services/AuthService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>

class LoginDialog::Private {
public:
    QLabel* titleLbl = nullptr;
    QLabel* subtitleLbl = nullptr;
    QLabel* userLbl = nullptr;
    QLabel* pwdLbl = nullptr;
    QLabel* errLbl = nullptr;
    QLineEdit* userEdit = nullptr;
    QLineEdit* pwdEdit = nullptr;
    QPushButton* loginBtn = nullptr;
    QPushButton* cancelBtn = nullptr;
    QCheckBox* showPwdCheck = nullptr;
};

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent), d(new Private) {
    setWindowTitle("监狱审讯笔录工具 - 登录");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(420, 380);
    setModal(true);
    setupUI();
    setStyleSheet(R"(
        QDialog {
            background-color: #0A0E14;
            border: 1px solid #21262D;
            border-radius: 10px;
        }
        #card {
            background-color: #0F1419;
            border: 1px solid #21262D;
            border-radius: 8px;
        }
        #title {
            color: #FFFFFF;
            font-size: 20px;
            font-weight: 700;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', sans-serif;
            qproperty-alignment: AlignCenter;
        }
        #subtitle {
            color: #8B949E;
            font-size: 12.5px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', sans-serif;
            qproperty-alignment: AlignCenter;
        }
        QLabel {
            color: #E6EDF3;
            font-size: 13.5px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
        }
        QLineEdit {
            padding: 9px 12px;
            border: 1px solid #21262D;
            border-radius: 4px;
            background: #0F1419;
            color: #E6EDF3;
            font-size: 13.5px;
            min-height: 20px;
            selection-background-color: #0066CC;
        }
        QLineEdit:focus {
            border: 1px solid #0066CC;
            background: #161B22;
        }
        QLineEdit:disabled {
            color: #6E7681;
            background: #0A0E14;
        }
        #loginBtn {
            background: #0066CC;
            color: #FFFFFF;
            border: none;
            border-radius: 4px;
            padding: 10px 28px;
            font-size: 14px;
            font-weight: 600;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            min-width: 90px;
        }
        #loginBtn:hover {
            background: #0052A3;
        }
        #loginBtn:pressed {
            background: #003D82;
        }
        #loginBtn:disabled {
            background: #161B22;
            color: #6E7681;
        }
        QPushButton {
            background: #1C2128;
            color: #E6EDF3;
            border: 1px solid #21262D;
            border-radius: 4px;
            padding: 10px 18px;
            font-size: 13.5px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            min-width: 80px;
        }
        QPushButton:hover {
            background: #30363D;
            border-color: #30363D;
        }
        QPushButton:pressed {
            background: #161B22;
        }
        #err {
            color: #E53E3E;
            font-size: 12.5px;
            min-height: 18px;
            qproperty-alignment: AlignCenter;
            font-weight: 500;
        }
        QCheckBox {
            color: #8B949E;
            font-size: 12.5px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', sans-serif;
        }
        QCheckBox::indicator {
            width: 15px;
            height: 15px;
            border-radius: 3px;
            border: 1.5px solid #30363D;
            background: #0F1419;
        }
        QCheckBox::indicator:checked {
            background: #0066CC;
            border-color: #0066CC;
        }
        QCheckBox::indicator:hover {
            border-color: #0066CC;
        }
        #accentBar {
            background: #0066CC;
            border-radius: 10px 10px 0 0;
        }
    )");
}

LoginDialog::~LoginDialog() { delete d; }

void LoginDialog::setupUI() {
    // 外层垂直布局
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // 顶部渐变色条
    QFrame* accentBar = new QFrame;
    accentBar->setObjectName("accentBar");
    accentBar->setFixedHeight(6);
    outer->addWidget(accentBar);

    // 主内容卡片
    QWidget* card = new QWidget;
    card->setObjectName("card");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 24, 32, 24);
    cardLayout->setSpacing(16);

    // 标题
    d->titleLbl = new QLabel("监狱审讯笔录工具");
    d->titleLbl->setObjectName("title");
    d->subtitleLbl = new QLabel("PrisonSIS · 安全管理信息系统");
    d->subtitleLbl->setObjectName("subtitle");

    // 用户名
    QGridLayout* formGrid = new QGridLayout;
    formGrid->setHorizontalSpacing(12);
    formGrid->setVerticalSpacing(14);

    d->userLbl = new QLabel("工号");
    d->userLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->userEdit = new QLineEdit;
    d->userEdit->setPlaceholderText("请输入工号");
    d->userEdit->setFixedHeight(40);

    d->pwdLbl = new QLabel("密码");
    d->pwdLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->pwdEdit = new QLineEdit;
    d->pwdEdit->setPlaceholderText("请输入密码");
    d->pwdEdit->setEchoMode(QLineEdit::Password);
    d->pwdEdit->setFixedHeight(40);

    formGrid->addWidget(d->userLbl, 0, 0);
    formGrid->addWidget(d->userEdit, 0, 1);
    formGrid->addWidget(d->pwdLbl, 1, 0);
    formGrid->addWidget(d->pwdEdit, 1, 1);

    // 记住密码
    d->showPwdCheck = new QCheckBox("显示密码");
    d->showPwdCheck->setCursor(Qt::PointingHandCursor);
    connect(d->showPwdCheck, &QCheckBox::toggled, this, [=](bool checked) {
        d->pwdEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    // 错误提示
    d->errLbl = new QLabel;
    d->errLbl->setObjectName("err");
    d->errLbl->setFixedHeight(22);

    // 按钮行
    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    d->loginBtn = new QPushButton("登录");
    d->loginBtn->setObjectName("loginBtn");
    d->loginBtn->setDefault(true);
    d->loginBtn->setCursor(Qt::PointingHandCursor);
    d->cancelBtn = new QPushButton("取消");
    d->cancelBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(d->cancelBtn);
    btnRow->addSpacing(10);
    btnRow->addWidget(d->loginBtn);

    // 组装
    cardLayout->addWidget(d->titleLbl);
    cardLayout->addWidget(d->subtitleLbl);
    cardLayout->addSpacing(8);
    cardLayout->addLayout(formGrid);
    cardLayout->addWidget(d->showPwdCheck);
    cardLayout->addWidget(d->errLbl);
    cardLayout->addSpacing(6);
    cardLayout->addLayout(btnRow);

    outer->addWidget(card);
    outer->addStretch();

    // 信号连接
    connect(d->loginBtn, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(d->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(d->pwdEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
    connect(d->userEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
}

void LoginDialog::onLogin() {
    QString user = d->userEdit->text().trimmed();
    QString pwd  = d->pwdEdit->text();

    d->errLbl->setText("");
    if (user.isEmpty()) {
        d->errLbl->setText("工号不能为空");
        d->userEdit->setFocus();
        return;
    }
    if (pwd.isEmpty()) {
        d->errLbl->setText("密码不能为空");
        d->pwdEdit->setFocus();
        return;
    }

    d->loginBtn->setEnabled(false);
    d->loginBtn->setText("登录中...");

    if (AuthService::instance().login(user, pwd)) {
        accept();
    } else {
        d->errLbl->setText("工号或密码错误");
        d->loginBtn->setEnabled(true);
        d->loginBtn->setText("登录");
        d->pwdEdit->clear();
        d->pwdEdit->setFocus();
    }
}

void LoginDialog::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Escape) reject();
    else QDialog::keyPressEvent(e);
}
