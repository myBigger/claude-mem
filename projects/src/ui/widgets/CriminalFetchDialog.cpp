#include "ui/widgets/CriminalFetchDialog.h"
#include "services/PrisonApiService.h"
#include "database/DatabaseManager.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "utils/DateUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QCheckBox>
#include <QMessageBox>
#include <QGroupBox>
#include <QAbstractItemView>
#include <QDebug>
#include <QDateTime>

CriminalFetchDialog::CriminalFetchDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("从内网获取罪犯信息");
    setMinimumSize(900, 650);
    resize(960, 720);
    setupUI();

    PrisonApiService& api = PrisonApiService::instance();
    connect(&api, &PrisonApiService::fetchSuccess,  this, &CriminalFetchDialog::onApiSuccess);
    connect(&api, &PrisonApiService::fetchError,   this, &CriminalFetchDialog::onApiError);
    connect(&api, &PrisonApiService::loginSuccess, this, &CriminalFetchDialog::onLoginSuccess);
    connect(&api, &PrisonApiService::loginFailed,  this, &CriminalFetchDialog::onLoginFailed);
    connect(&api, &PrisonApiService::requestStarted,  this, [=](){ setLoading(true); });
    connect(&api, &PrisonApiService::requestFinished, this, [=](){ setLoading(false); });

    // 根据是否已登录决定显示哪个面板
    if (api.isLoggedIn()) {
        showSearchPanel();
    } else {
        showLoginPanel();
    }
}

CriminalFetchDialog::~CriminalFetchDialog() {}

void CriminalFetchDialog::showLoginPanel() {
    if (m_searchPanel) m_searchPanel->hide();
    if (m_loginPanel) { m_loginPanel->show(); return; }

    PrisonApiService& api = PrisonApiService::instance();
    api.loadConfig();

    m_loginPanel = new QWidget(this);
    m_loginPanel->setStyleSheet(R"(
        QWidget{background:#0A0E14;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit{padding:10px 14px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:14px;font-family:'Microsoft YaHei',sans-serif;}
        QLineEdit:focus{border-color:#0066CC;}
        QGroupBox{font-weight:600;font-size:13px;color:#0066CC;border:1px solid #21262D;border-radius:8px;margin-top:8px;background:#0F1419;padding-top:8px;text-transform:uppercase;letter-spacing:0.5px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 8px;background:#0F1419;}
        QFrame#btnFrame{background:#0F1419;border-top:1px solid #21262D;}
        QPushButton#loginBtn{background:#0066CC;color:#FFFFFF;font-weight:600;font-size:14px;border:none;border-radius:4px;padding:10px 24px;}
        QPushButton#loginBtn:hover{background:#0052A3;}
        QPushButton#loginBtn:pressed{background:#003D82;}
        QPushButton#cancelBtn{background:#1C2128;color:#E6EDF3;border:none;border-radius:4px;padding:10px 20px;font-size:14px;}
        QPushButton#cancelBtn:hover{background:#30363D;}
    )");
    QVBoxLayout* v = new QVBoxLayout(m_loginPanel);
    v->setContentsMargins(40, 30, 40, 30);
    v->setSpacing(20);

    // 标题
    QLabel* title = new QLabel("狱政平台单点登录");
    title->setStyleSheet("font-size:22px;font-weight:600;color:#FFFFFF;letter-spacing:-0.3px;");
    title->setAlignment(Qt::AlignCenter);
    v->addWidget(title);

    QLabel* subtitle = new QLabel("通过 CAS 单点登录访问监狱内网狱政管理平台");
    subtitle->setStyleSheet("font-size:13px;color:#8B949E;");
    subtitle->setAlignment(Qt::AlignCenter);
    v->addWidget(subtitle);
    v->addSpacing(10);

    // 登录表单
    QGroupBox* formBox = new QGroupBox("统一认证登录");
    QGridLayout* grid = new QGridLayout(formBox);
    grid->setContentsMargins(20, 20, 20, 20);
    grid->setHorizontalSpacing(15);
    grid->setVerticalSpacing(12);
    grid->setColumnStretch(1, 1);

    QLabel* casLabel = new QLabel("CAS 地址：");
    m_casUrlEdit = new QLineEdit;
    m_casUrlEdit->setPlaceholderText("http://内网CAS地址:端口/cas");
    m_casUrlEdit->setText(api.baseUrl().isEmpty() ? "http://192.168.1.100:8080/cas" : api.baseUrl());

    QLabel* userLabel = new QLabel("用户名：");
    m_usernameEdit = new QLineEdit;
    m_usernameEdit->setPlaceholderText("请输入内网统一平台用户名");

    QLabel* pwdLabel = new QLabel("密码：");
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText("请输入密码（将使用RSA加密传输）");
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    grid->addWidget(casLabel,  0, 0, Qt::AlignRight);
    grid->addWidget(m_casUrlEdit,  0, 1);
    grid->addWidget(userLabel, 1, 0, Qt::AlignRight);
    grid->addWidget(m_usernameEdit, 1, 1);
    grid->addWidget(pwdLabel,  2, 0, Qt::AlignRight);
    grid->addWidget(m_passwordEdit,  2, 1);

    v->addWidget(formBox);

    m_loginStatusLabel = new QLabel("请输入统一平台用户名和密码登录");
    m_loginStatusLabel->setStyleSheet("color:#6E7681;font-size:12px;padding:4px;");
    m_loginStatusLabel->setAlignment(Qt::AlignCenter);
    v->addWidget(m_loginStatusLabel);

    // 按钮
    QFrame* btnFrame = new QFrame;
    btnFrame->setObjectName("btnFrame");
    btnFrame->setFixedHeight(60);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnFrame);
    btnLayout->setContentsMargins(15, 10, 15, 10);

    QLabel* spacer = new QLabel;
    btnLayout->addWidget(spacer, 1);

    m_loginBtn = new QPushButton("登录并获取罪犯数据");
    m_loginBtn->setObjectName("loginBtn");
    m_loginBtn->setFixedSize(200, 38);
    connect(m_loginBtn, &QPushButton::clicked, this, &CriminalFetchDialog::onLogin);

    QPushButton* cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setFixedSize(100, 38);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(m_loginBtn);
    v->addWidget(btnFrame);

    // 回车登录
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &CriminalFetchDialog::onLogin);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, &CriminalFetchDialog::onLogin);

    // 设置焦点
    m_usernameEdit->setFocus();
}

void CriminalFetchDialog::showSearchPanel() {
    if (m_loginPanel) m_loginPanel->hide();
    if (m_searchPanel) { m_searchPanel->show(); return; }
    // m_searchPanel 由 setupUI() 构建，直接显示
}

void CriminalFetchDialog::setupUI() {
    m_searchPanel = new QWidget(this);
    m_searchPanel->setStyleSheet(R"(
        QWidget{background-color:#0A0E14;}
        QLabel{color:#E6EDF3;font-size:13px;}
        QLineEdit{padding:6px 8px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;}
        QLineEdit:focus{border:1.5px solid #0066CC;}
        QGroupBox{font-weight:600;font-size:12px;color:#0066CC;border:1px solid #21262D;border-radius:6px;margin-top:8px;background:#0F1419;text-transform:uppercase;letter-spacing:0.5px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 6px;background:#0F1419;}
        QTableWidget{background:#0A0E14;color:#E6EDF3;font-size:12px;gridline-color:#21262D;border:1px solid #21262D;}
        QTableWidget::item{color:#E6EDF3;}
        QTableWidget::item:selected{background:#0066CC1A;color:#FFFFFF;font-weight:600;}
        QHeaderView::section{background:#161B22;color:#8B949E;font-weight:600;padding:6px;border-bottom:1px solid #21262D;border-right:1px solid #21262D;text-transform:uppercase;letter-spacing:0.5px;}
        QFrame#btnFrame2{background:#0F1419;border-top:1px solid #21262D;}
        QPushButton{background:#0066CC;color:#FFFFFF;border:none;border-radius:4px;font-size:13px;font-weight:600;padding:6px 12px;}
        QPushButton:hover{background:#0052A3;}
        QPushButton:disabled{background:#161B22;color:#6E7681;}
    )");
    QVBoxLayout* main = new QVBoxLayout(m_searchPanel);
    main->setContentsMargins(15, 12, 15, 12);
    main->setSpacing(10);
    this->setLayout(main);

    // === 登录状态横幅 ===
    PrisonApiService& api = PrisonApiService::instance();
    QFrame* statusBar = new QFrame;
    statusBar->setStyleSheet("background:#161B22;border:1px solid #21262D;border-radius:8px;padding:6px 12px;");
    QHBoxLayout* sbLayout = new QHBoxLayout(statusBar);
    sbLayout->setContentsMargins(8,4,8,4);
    sbLayout->addWidget(new QLabel("✅ 已通过 CAS 统一认证"));
    sbLayout->addWidget(new QLabel("CAS: " + api.baseUrl()), 1);
    QPushButton* logoutBtn = new QPushButton("退出登录");
    logoutBtn->setStyleSheet(R"(
        QPushButton{background:#E53E3E;color:#FFFFFF;border:none;border-radius:4px;font-size:11px;padding:4px 12px;font-weight:600;}
        QPushButton:hover{background:#C53030;}
    )");
    connect(logoutBtn, &QPushButton::clicked, this, [=, &api](){
        api.logout();
        showLoginPanel();
    });
    sbLayout->addWidget(logoutBtn);
    main->addWidget(statusBar);

    // === 提示横幅 ===
    QFrame* tipBar = new QFrame;
    tipBar->setStyleSheet("background:#161B22;border:1px solid #21262D;border-radius:8px;padding:6px 10px;");
    QLabel* tipText = new QLabel("通过内网 API 自动获取罪犯基础信息，无需手动录入，避免输入错误。");
    tipText->setStyleSheet("color:#8B949E;font-size:12px;");
    QHBoxLayout* tipLayout = new QHBoxLayout(tipBar);
    tipLayout->setContentsMargins(8,4,8,4);
    tipLayout->addWidget(new QLabel("💡"));
    tipLayout->addWidget(tipText, 1);
    main->addWidget(tipBar);

    // === 查询区 ===
    QGroupBox* searchBox = new QGroupBox("查询条件");
    QGridLayout* searchGrid = new QGridLayout(searchBox);
    searchGrid->setContentsMargins(14, 18, 14, 14);
    searchGrid->setHorizontalSpacing(10);
    searchGrid->setVerticalSpacing(8);

    QLabel* idLabel = new QLabel("罪犯编号：");
    m_idEdit = new QLineEdit;
    m_idEdit->setPlaceholderText("输入精确编号，如 FZ-2024-0001");
    m_searchIdBtn = new QPushButton("精确查询");
    m_searchIdBtn->setFixedSize(90, 32);
    connect(m_searchIdBtn, &QPushButton::clicked, this, &CriminalFetchDialog::onSearchById);
    connect(m_idEdit, &QLineEdit::returnPressed, this, &CriminalFetchDialog::onSearchById);

    QLabel* nameLabel = new QLabel("姓名（模糊）：");
    m_nameEdit = new QLineEdit;
    m_nameEdit->setPlaceholderText("输入罪犯姓名，支持模糊匹配");
    m_searchNameBtn = new QPushButton("姓名搜索");
    m_searchNameBtn->setFixedSize(90, 32);
    connect(m_searchNameBtn, &QPushButton::clicked, this, &CriminalFetchDialog::onSearchByName);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &CriminalFetchDialog::onSearchByName);

    QPushButton* syncBtn = new QPushButton("同步一页(20条)");
    syncBtn->setFixedSize(110, 32);
    connect(syncBtn, &QPushButton::clicked, this, [=](){
        PrisonApiService::instance().syncPage(1, 20);
    });

    searchGrid->addWidget(idLabel,       0, 0, Qt::AlignRight);
    searchGrid->addWidget(m_idEdit,        0, 1);
    searchGrid->addWidget(m_searchIdBtn,   0, 2);
    searchGrid->addWidget(syncBtn,         0, 3, Qt::AlignLeft);
    searchGrid->addWidget(nameLabel,       1, 0, Qt::AlignRight);
    searchGrid->addWidget(m_nameEdit,      1, 1);
    searchGrid->addWidget(m_searchNameBtn, 1, 2);
    main->addWidget(searchBox);

    // === 结果表格 ===
    m_table = new QTableWidget;
    m_table->setColumnCount(12);
    m_table->setHorizontalHeaderLabels({
        "选择", "罪犯编号", "姓名", "性别", "民族",
        "罪名", "刑期", "监区", "监室", "入狱日期", "身份证号", "管理等级"
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    main->addWidget(m_table, 1);

    // === 状态栏 ===
    m_statusLabel = new QLabel("请输入查询条件查询罪犯信息");
    main->addWidget(m_statusLabel);

    // === 底部按钮栏 ===
    QFrame* btnFrame = new QFrame;
    btnFrame->setObjectName("btnFrame2");
    btnFrame->setFixedHeight(52);
    QHBoxLayout* btnLayout = new QHBoxLayout(btnFrame);
    btnLayout->setContentsMargins(15, 8, 15, 8);
    btnLayout->setSpacing(10);

    QPushButton* selAllBtn = new QPushButton("全选");
    selAllBtn->setFixedSize(80, 34);
    connect(selAllBtn, &QPushButton::clicked, this, &CriminalFetchDialog::onSelectAll);

    QPushButton* clearSelBtn = new QPushButton("取消全选");
    clearSelBtn->setFixedSize(80, 34);
    connect(clearSelBtn, &QPushButton::clicked, this, &CriminalFetchDialog::onClearSelection);

    btnLayout->addWidget(selAllBtn);
    btnLayout->addWidget(clearSelBtn);
    btnLayout->addWidget(new QLabel, 1);

    m_importBtn = new QPushButton("导入选中 (0)");
    m_importBtn->setFixedSize(150, 34);
    m_importBtn->setEnabled(false);
    connect(m_importBtn, &QPushButton::clicked, this, &CriminalFetchDialog::onImportSelected);

    QPushButton* closeBtn = new QPushButton("关闭");
    closeBtn->setFixedSize(80, 34);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_importBtn);
    btnLayout->addWidget(closeBtn);
    main->addWidget(btnFrame);
}

void CriminalFetchDialog::onLogin() {
    QString casUrl = m_casUrlEdit->text().trimmed();
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }
    if (casUrl.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入 CAS 地址");
        return;
    }

    m_loginBtn->setEnabled(false);
    m_loginStatusLabel->setText("正在连接 CAS 并加密认证，请稍候……");
    m_loginStatusLabel->setStyleSheet("color:#F59E0B;font-size:12px;font-weight:600;");

    PrisonApiService& api = PrisonApiService::instance();
    api.loadConfig();
    api.initAndLogin(casUrl, username, password);
}

void CriminalFetchDialog::onLoginSuccess() {
    m_loginStatusLabel->setText("✅ 登录成功，正在加载罪犯数据……");
    m_loginStatusLabel->setStyleSheet("color:#22C55E;font-size:12px;font-weight:600;");
    PrisonApiService::instance().loadConfig();
    PrisonApiService::instance().saveConfig();
    showSearchPanel();
    // 登录成功后自动搜索
    PrisonApiService::instance().syncPage(1, 20);
}

void CriminalFetchDialog::onLoginFailed(const QString& err) {
    m_loginBtn->setEnabled(true);
    m_loginStatusLabel->setText("❌ 登录失败：" + err);
    m_loginStatusLabel->setStyleSheet("color:#E53E3E;font-size:12px;font-weight:600;");
    QMessageBox::warning(this, "登录失败", err);
}

void CriminalFetchDialog::setLoading(bool on) {
    if (m_searchIdBtn) m_searchIdBtn->setEnabled(!on);
    if (m_searchNameBtn) m_searchNameBtn->setEnabled(!on);
    if (on && m_statusLabel) {
        m_statusLabel->setText("正在从内网获取数据，请稍候……");
        m_statusLabel->setStyleSheet("color:#F59E0B;font-size:12px;font-weight:600;");
    }
}

void CriminalFetchDialog::onSearchById() {
    QString id = m_idEdit->text().trimmed();
    if (id.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入罪犯编号");
        return;
    }
    PrisonApiService::instance().fetchById(id);
}

void CriminalFetchDialog::onSearchByName() {
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入罪犯姓名");
        return;
    }
    PrisonApiService::instance().searchByName(name);
}

void CriminalFetchDialog::onApiSuccess(const QJsonArray& results) {
    m_currentResults = results;
    populateTable(results);
    if (m_statusLabel) {
        m_statusLabel->setText(QString("查到 %1 条记录，请勾选要导入的罪犯").arg(results.size()));
        m_statusLabel->setStyleSheet("color:#22C55E;font-size:12px;font-weight:600;");
    }
}

void CriminalFetchDialog::onApiError(const QString& err) {
    if (m_statusLabel) {
        m_statusLabel->setText("查询失败：" + err);
        m_statusLabel->setStyleSheet("color:#E53E3E;font-size:12px;font-weight:600;");
    }
    QMessageBox::warning(this, "查询失败", err);
}

void CriminalFetchDialog::populateTable(const QJsonArray& results) {
    if (!m_table) return;
    m_table->setRowCount(0);
    for (int i = 0; i < results.size(); ++i) {
        QJsonObject obj = results[i].toObject();
        m_table->insertRow(i);

        // 复选框
        QWidget* checkWidget = new QWidget;
        QHBoxLayout* hbox = new QHBoxLayout(checkWidget);
        hbox->setContentsMargins(4,2,4,2);
        hbox->setAlignment(Qt::AlignCenter);
        QCheckBox* cb = new QCheckBox;
        hbox->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, [=](bool){
            updateImportBtnText();
        });
        m_table->setCellWidget(i, 0, checkWidget);

        auto cell = [&](int col, const QString& val) {
            QTableWidgetItem* item = new QTableWidgetItem(val);
            item->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(i, col, item);
        };

        cell(1,  obj.value("criminal_id").toString());
        cell(2,  obj.value("name").toString());
        cell(3,  obj.value("gender").toString());
        cell(4,  obj.value("ethnicity").toString());
        cell(5,  obj.value("crime").toString());
        int sy = obj.value("sentence_years").toInt();
        int sm = obj.value("sentence_months").toInt();
        QString sent;
        if (sy > 0) sent += QString::number(sy) + "年";
        if (sm > 0) sent += QString::number(sm) + "月";
        cell(6, sent.isEmpty() ? "-" : sent);
        cell(7,  obj.value("district").toString());
        cell(8,  obj.value("cell").toString());
        cell(9,  obj.value("entry_date").toString());
        cell(10, obj.value("id_card_number").toString());
        cell(11, obj.value("manage_level").toString());
    }
    m_table->resizeColumnsToContents();
    m_table->setColumnWidth(1, 140);
    m_table->setColumnWidth(2, 80);
    m_table->setColumnWidth(6, 70);
    m_table->setColumnWidth(9, 100);
    m_table->setColumnWidth(10, 160);
}

void CriminalFetchDialog::updateImportBtnText() {
    if (!m_table || !m_importBtn) return;
    int cnt = 0;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* w = qobject_cast<QCheckBox*>(m_table->cellWidget(r, 0)->findChild<QCheckBox*>());
        if (w && w->isChecked()) ++cnt;
    }
    m_importBtn->setText(QString("导入选中 (%1)").arg(cnt));
    m_importBtn->setEnabled(cnt > 0);
}

void CriminalFetchDialog::onSelectAll() {
    if (!m_table) return;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* w = qobject_cast<QCheckBox*>(m_table->cellWidget(r, 0)->findChild<QCheckBox*>());
        if (w) w->setChecked(true);
    }
}

void CriminalFetchDialog::onClearSelection() {
    if (!m_table) return;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* w = qobject_cast<QCheckBox*>(m_table->cellWidget(r, 0)->findChild<QCheckBox*>());
        if (w) w->setChecked(false);
    }
}

void CriminalFetchDialog::onImportSelected() {
    if (!m_table) return;
    QStringList imported, failed;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* w = qobject_cast<QCheckBox*>(m_table->cellWidget(r, 0)->findChild<QCheckBox*>());
        if (!w || !w->isChecked()) continue;
        if (r >= m_currentResults.size()) continue;

        QJsonObject obj = m_currentResults[r].toObject();
        QString cid = obj.value("criminal_id").toString();
        QString name = obj.value("name").toString();
        if (cid.isEmpty()) { failed.append(name.isEmpty() ? "(无编号)" : name); continue; }

        auto existing = DatabaseManager::instance().selectOne(
            "SELECT id FROM criminals WHERE criminal_id=:id", {{"id", cid}});
        if (!existing.isEmpty()) {
            failed.append(QString("%1(%2) - 已存在").arg(name).arg(cid)); continue;
        }

        QMap<QString, QVariant> vals;
        vals["criminal_id"]     = cid;
        vals["name"]            = name;
        vals["gender"]          = obj.value("gender").toString();
        vals["ethnicity"]       = obj.value("ethnicity").toString();
        vals["birth_date"]      = obj.value("birth_date").toString();
        vals["id_card_number"] = obj.value("id_card_number").toString();
        vals["native_place"]    = obj.value("native_place").toString();
        vals["education"]       = obj.value("education").toString();
        vals["crime"]           = obj.value("crime").toString();
        vals["sentence_years"]  = obj.value("sentence_years").toInt(0);
        vals["sentence_months"]= obj.value("sentence_months").toInt(0);
        vals["entry_date"]      = obj.value("entry_date").toString();
        vals["original_court"] = obj.value("original_court").toString();
        vals["district"]        = obj.value("district").toString();
        vals["cell"]            = obj.value("cell").toString();
        vals["crime_type"]      = obj.value("crime_type").toString();
        vals["manage_level"]    = obj.value("manage_level").toString();
        vals["remark"]          = obj.value("remark").toString();
        vals["archived"]        = 0;
        vals["created_at"]      = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        vals["updated_at"]      = vals["created_at"];

        int newId = DatabaseManager::instance().insert("criminals", vals);
        if (newId > 0) {
            imported.append(QString("%1(%2)").arg(name).arg(cid));
            AuditService::instance().log(
                AuthService::instance().currentUser().id,
                AuthService::instance().currentUser().realName,
                AuditService::ACTION_CREATE, "罪犯管理", "Criminal", newId,
                QString("从内网API导入:%1/%2").arg(cid).arg(name));
        } else {
            failed.append(QString("%1(%2) - 写入失败").arg(name).arg(cid));
        }
    }

    QString msg;
    if (!imported.isEmpty()) msg += QString("✅ 成功导入 %1 条：\n%2\n\n").arg(imported.size()).arg(imported.join("、"));
    if (!failed.isEmpty())  msg += QString("❌ 失败 %1 条：\n%2").arg(failed.size()).arg(failed.join("、"));
    QMessageBox::information(this, "导入结果", msg.isEmpty() ? "未选中任何记录" : msg);
    if (!imported.isEmpty()) accept();
}

QStringList CriminalFetchDialog::selectedCriminalIds() const {
    if (!m_table) return {};
    QStringList ids;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* w = qobject_cast<QCheckBox*>(m_table->cellWidget(r, 0)->findChild<QCheckBox*>());
        if (w && w->isChecked() && r < m_currentResults.size()) {
            ids.append(m_currentResults[r].toObject().value("criminal_id").toString());
        }
    }
    return ids;
}