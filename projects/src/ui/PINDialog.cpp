#include "ui/PINDialog.h"
#include "services/KeyManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>

PINDialog::PINDialog(QWidget* parent)
    : QDialog(parent), m_unlockOnly(false)
{
    setupSetupMode();
}

PINDialog::PINDialog(bool unlockOnly, QWidget* parent)
    : QDialog(parent), m_unlockOnly(unlockOnly)
{
    if (unlockOnly) setupUnlockMode();
    else            setupSetupMode();
}

PINDialog::~PINDialog() = default;

void PINDialog::setupSetupMode() {
    setWindowTitle("设置数据库保护 PIN");
    setFixedSize(440, 300);
    setModal(true);
    setStyleSheet(R"(
        QDialog{background:#0A0E14;border:1px solid #21262D;border-radius:12px;}
        #card{background:#0F1419;border:1px solid #21262D;border-radius:10px;padding:24px 28px;}
        #title{color:#FFFFFF;font-size:17px;font-weight:bold;qproperty-alignment:'AlignCenter';padding-bottom:4px;}
        #desc{color:#8B949E;font-size:13px;qproperty-alignment:'AlignCenter';padding:0 12px 16px;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit{
            padding:10px 14px;border:1.5px solid #21262D;border-radius:4px;
            background:#0F1419;color:#E6EDF3;font-size:14px;min-height:20px;
            selection-background-color:#0066CC;
        }
        QLineEdit:focus{border-color:#0066CC;background:#161B22;}
        #msgLabel{color:#E53E3E;font-size:13px;qproperty-alignment:'AlignCenter';font-weight:500;min-height:20px;}
        #submitBtn{
            background:#0066CC;
            color:#FFFFFF;border:none;border-radius:4px;padding:10px 28px;font-size:15px;font-weight:bold;
        }
        #submitBtn:hover{background:#0052A3;}
        #submitBtn:pressed{background:#003D82;}
        #cancelBtn{background:#1C2128;color:#E6EDF3;border:1px solid #21262D;border-radius:4px;padding:10px 20px;font-size:14px;}
        #cancelBtn:hover{background:#30363D;border-color:#30363D;}
    )");

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);

    // 顶部渐变条
    QFrame* accentBar = new QFrame;
    accentBar->setStyleSheet("background:#0066CC;border-radius:12px 12px 0 0;");
    accentBar->setFixedHeight(4);
    main->addWidget(accentBar);

    QWidget* card = new QWidget;
    card->setObjectName("card");
    QVBoxLayout* cardV = new QVBoxLayout(card);
    cardV->setSpacing(14);

    QLabel* title = new QLabel("设置数据库保护 PIN");
    title->setObjectName("title");
    QLabel* desc = new QLabel(
        "此 PIN 用于加密保护数据库中的笔录内容。\n"
        "遗忘 PIN 将导致数据库无法访问。\n"
        "请务必记住您的 PIN。");
    desc->setObjectName("desc");
    desc->setWordWrap(true);

    QFormLayout* form = new QFormLayout;
    form->setSpacing(10);
    m_pinEdit = new QLineEdit;
    m_pinEdit->setPlaceholderText("输入 6 位数字 PIN");
    m_pinEdit->setEchoMode(QLineEdit::Password);
    m_pinEdit->setMaxLength(6);
    m_confirmEdit = new QLineEdit;
    m_confirmEdit->setPlaceholderText("再次输入 PIN 确认");
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setMaxLength(6);
    form->addRow("设置 PIN：", m_pinEdit);
    form->addRow("确认 PIN：", m_confirmEdit);

    m_msgLabel = new QLabel("");
    m_msgLabel->setObjectName("msgLabel");

    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton* cancelBtn = new QPushButton("取消（程序将退出）");
    cancelBtn->setObjectName("cancelBtn");
    m_submitBtn = new QPushButton("确认设置");
    m_submitBtn->setObjectName("submitBtn");
    m_submitBtn->setDefault(true);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_submitBtn);

    cardV->addWidget(title);
    cardV->addWidget(desc);
    cardV->addLayout(form);
    cardV->addWidget(m_msgLabel);
    cardV->addSpacing(4);
    cardV->addLayout(btnRow);

    main->addWidget(card);

    connect(m_submitBtn, &QPushButton::clicked, this, &PINDialog::onSubmit);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void PINDialog::setupUnlockMode() {
    setWindowTitle("解锁数据库");
    setFixedSize(400, 240);
    setModal(true);
    setStyleSheet(R"(
        QDialog{background:#0A0E14;border:1px solid #21262D;border-radius:12px;}
        #card{background:#0F1419;border:1px solid #21262D;border-radius:10px;padding:24px 28px;}
        #title{color:#FFFFFF;font-size:17px;font-weight:bold;qproperty-alignment:'AlignCenter';}
        #desc{color:#8B949E;font-size:13px;qproperty-alignment:'AlignCenter';padding-bottom:16px;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit{
            padding:10px 14px;border:1.5px solid #21262D;border-radius:4px;
            background:#0F1419;color:#E6EDF3;font-size:14px;min-height:20px;
            selection-background-color:#0066CC;
        }
        QLineEdit:focus{border-color:#0066CC;background:#161B22;}
        #msgLabel{color:#E53E3E;font-size:13px;qproperty-alignment:'AlignCenter';font-weight:500;min-height:20px;}
        #submitBtn{
            background:#0066CC;
            color:#FFFFFF;border:none;border-radius:4px;padding:10px 28px;font-size:15px;font-weight:bold;
        }
        #submitBtn:hover{background:#0052A3;}
        #submitBtn:pressed{background:#003D82;}
        #cancelBtn{background:#1C2128;color:#E6EDF3;border:1px solid #21262D;border-radius:4px;padding:10px 20px;font-size:14px;}
        #cancelBtn:hover{background:#30363D;border-color:#30363D;}
    )");

    QVBoxLayout* main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);

    QFrame* accentBar = new QFrame;
    accentBar->setStyleSheet("background:#0066CC;border-radius:12px 12px 0 0;");
    accentBar->setFixedHeight(4);
    main->addWidget(accentBar);

    QWidget* card = new QWidget;
    card->setObjectName("card");
    QVBoxLayout* cardV = new QVBoxLayout(card);
    cardV->setSpacing(14);

    QLabel* title = new QLabel("请输入 PIN 解锁数据库");
    title->setObjectName("title");
    QLabel* desc = new QLabel(
        "数据库内容已加密保护。\n"
        "请输入您的 PIN 码解锁。");
    desc->setObjectName("desc");
    desc->setWordWrap(true);

    QFormLayout* form = new QFormLayout;
    form->setSpacing(10);
    m_pinEdit = new QLineEdit;
    m_pinEdit->setPlaceholderText("输入 PIN 码");
    m_pinEdit->setEchoMode(QLineEdit::Password);
    m_pinEdit->setMaxLength(6);
    form->addRow("PIN 码：", m_pinEdit);

    m_msgLabel = new QLabel("");
    m_msgLabel->setObjectName("msgLabel");

    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    QPushButton* cancelBtn = new QPushButton("取消（程序将退出）");
    cancelBtn->setObjectName("cancelBtn");
    m_submitBtn = new QPushButton("解锁");
    m_submitBtn->setObjectName("submitBtn");
    m_submitBtn->setDefault(true);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(m_submitBtn);

    cardV->addWidget(title);
    cardV->addWidget(desc);
    cardV->addLayout(form);
    cardV->addWidget(m_msgLabel);
    cardV->addSpacing(4);
    cardV->addLayout(btnRow);

    main->addWidget(card);

    connect(m_submitBtn, &QPushButton::clicked, this, &PINDialog::onSubmit);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_pinEdit, &QLineEdit::returnPressed, this, &PINDialog::onSubmit);
}

void PINDialog::onSubmit() {
    QString pin = m_pinEdit->text();

    if (m_unlockOnly) {
        if (pin.length() < 6) {
            m_msgLabel->setText("PIN 长度不能少于 6 位");
            m_pinEdit->clear();
            return;
        }
        if (KeyManager::instance().unlock(pin)) {
            m_unlocked = true;
            accept();
        } else {
            ++m_attempts;
            if (m_attempts >= 3) {
                QMessageBox::critical(this, "错误",
                    QString("连续 %1 次 PIN 错误。\n请重启程序重试，或联系管理员。").arg(m_attempts));
                reject();
                return;
            }
            m_msgLabel->setText(QString("PIN 错误，剩余尝试次数：%1").arg(3 - m_attempts));
            m_pinEdit->clear();
            m_pinEdit->setFocus();
        }
    } else {
        if (pin.length() < 6) {
            m_msgLabel->setText("PIN 长度不能少于 6 位");
            return;
        }
        if (pin != m_confirmEdit->text()) {
            m_msgLabel->setText("两次输入的 PIN 不一致");
            m_confirmEdit->clear();
            return;
        }
        if (KeyManager::instance().setup(pin)) {
            m_unlocked = true;
            QMessageBox::information(this, "成功",
                "数据库保护 PIN 已设置！\n\n"
                "请务必记住此 PIN，遗忘将导致数据库无法访问。\n"
                "建议将 PIN 手写在安全的地方。");
            accept();
        } else {
            m_msgLabel->setText("设置失败，请重试");
        }
    }
}
