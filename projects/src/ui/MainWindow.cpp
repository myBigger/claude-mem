#include "ui/MainWindow.h"
#include "ui/widgets/CriminalListWidget.h"
#include "ui/widgets/RecordEditorWidget.h"
#include "ui/widgets/ApprovalWidget.h"
#include "ui/widgets/ArchiveWidget.h"
#include "ui/widgets/StatisticsWidget.h"
#include "ui/widgets/TemplateWidget.h"
#include "ui/widgets/UserManageWidget.h"
#include "ui/widgets/LogViewerWidget.h"
#include "ui/widgets/BackupWidget.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "services/P2PNodeService.h"
#include "services/KeyManager.h"
#include "database/DatabaseManager.h"
#include <QCloseEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTextBrowser>
#include <QTimer>
#include <QPushButton>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("监狱审讯笔录工具 v1.0.0");
    // GNOME Wayland 下逐个设置窗口标志，避免 OR 组合导致部分标志被覆盖
    // 先清除所有自定义标志，再逐个开启所需按钮
    setWindowFlags(Qt::Window);
    setWindowFlag(Qt::WindowMinimizeButtonHint, true);
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);
    setWindowFlag(Qt::WindowCloseButtonHint, true);
    setMinimumSize(1280, 720);
    resize(1400, 800);
    setupMenuBar();
    setupNav();
    setupContent();
    setupStatusBar();
    setStyleSheet(R"(
        /* ── Government Blue Dark Design System ── */
        /* 融合 IBM Carbon + HashiCorp + Linear 设计规范 */
        /* 参考：DESIGN_SYSTEM.md — PrisonSIS Design System */

        /* 窗口背景 */
        QMainWindow {
            background: #0A0E14;
        }
        /* 侧边导航栏 */
        QTreeWidget {
            background: #161B22;
            border: none;
            border-right: 1px solid #21262D;
            padding: 8px 0;
            outline: none;
        }
        /* 分类标题 */
        QTreeWidget::item {
            color: #484F58;
            font-size: 11px;
            font-weight: 600;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            padding: 12px 18px 4px;
            min-height: 20px;
            text-transform: uppercase;
            letter-spacing: 0.8px;
        }
        /* 导航菜单项 */
        QTreeWidget::item {
            color: #8B949E;
            padding: 6px 18px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            min-height: 28px;
            border-radius: 0;
        }
        QTreeWidget::item:hover {
            background: #1C2128;
            color: #E6EDF3;
        }
        QTreeWidget::item:selected {
            background: #0066CC1A;
            color: #FFFFFF;
            border-left: 2px solid #0066CC;
            padding-left: 16px;
        }
        /* 状态栏 */
        QStatusBar {
            background: #0F1419;
            color: #8B949E;
            font-size: 12px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', sans-serif;
            border-top: 1px solid #21262D;
            padding: 4px 12px;
        }
        QStatusBar::item {
            border: none;
        }
        /* 菜单栏 */
        QMenuBar {
            background: #0F1419;
            color: #E6EDF3;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            border-bottom: 1px solid #21262D;
            padding: 3px 0;
        }
        QMenuBar::item {
            padding: 5px 12px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background: #1C2128;
            color: #FFFFFF;
        }
        /* 下拉菜单 */
        QMenu {
            background: #161B22;
            border: 1px solid #21262D;
            border-radius: 6px;
            padding: 4px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
        }
        QMenu::item {
            padding: 6px 14px;
            border-radius: 4px;
            color: #E6EDF3;
        }
        QMenu::item:selected {
            background: #1C2128;
            color: #FFFFFF;
        }
        QMenu::separator {
            height: 1px;
            background: #21262D;
            margin: 3px 6px;
        }
        /* 全局通用控件 */
        QLabel {
            color: #E6EDF3;
        }
        QPushButton {
            background: #1C2128;
            color: #E6EDF3;
            border: 1px solid #21262D;
            border-radius: 4px;
            padding: 6px 14px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
        }
        QPushButton:hover {
            background: #30363D;
            border-color: #30363D;
            color: #FFFFFF;
        }
        QPushButton:pressed {
            background: #161B22;
            border-color: #21262D;
        }
        QPushButton:disabled {
            background: #161B22;
            color: #6E7681;
            border-color: #21262D;
        }
        /* 主操作按钮（Government Blue #0066CC） */
        QPushButton#primaryBtn,
        QPushButton[objectName="primaryBtn"] {
            background: #0066CC;
            color: #FFFFFF;
            font-weight: 600;
            border: none;
            border-radius: 4px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
        }
        QPushButton#primaryBtn:hover,
        QPushButton[objectName="primaryBtn"]:hover {
            background: #0052A3;
            color: #FFFFFF;
        }
        QPushButton#primaryBtn:pressed,
        QPushButton[objectName="primaryBtn"]:pressed {
            background: #003D82;
        }
        /* 输入框 */
        QLineEdit, QTextEdit, QPlainTextEdit {
            background: #0F1419;
            color: #E6EDF3;
            border: 1px solid #21262D;
            border-radius: 4px;
            padding: 7px 10px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            selection-background-color: #0066CC;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border-color: #0066CC;
            background: #161B22;
        }
        QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
            background: #0A0E14;
            color: #6E7681;
        }
        /* 下拉框 */
        QComboBox {
            background: #0F1419;
            color: #E6EDF3;
            border: 1px solid #21262D;
            border-radius: 4px;
            padding: 7px 10px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
        }
        QComboBox:hover {
            border-color: #30363D;
        }
        QComboBox:focus {
            border-color: #0066CC;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background: #161B22;
            color: #E6EDF3;
            border: 1px solid #21262D;
            border-radius: 4px;
            selection-background-color: #1C2128;
            padding: 4px;
        }
        /* 表格 — Government Blue 主题：交替行、数据密集 */
        QTableWidget, QTreeWidget {
            background: #0A0E14;
            alternate-background-color: #0F1419;
            color: #E6EDF3;
            gridline-color: #21262D;
            border: 1px solid #21262D;
            border-radius: 6px;
            font-size: 13px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
        }
        QTableWidget::item, QTreeWidget::item {
            padding: 6px 8px;
            border-bottom: 1px solid #21262D;
        }
        QTableWidget::item:hover, QTreeWidget::item:hover {
            background: #161B22;
        }
        QTableWidget::item:selected, QTreeWidget::item:selected {
            background: #0066CC1A;
            color: #FFFFFF;
        }
        QHeaderView::section {
            background: #161B22;
            color: #8B949E;
            font-weight: 600;
            font-size: 12px;
            padding: 8px 10px;
            border: none;
            border-bottom: 1px solid #21262D;
            border-right: 1px solid #21262D;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QHeaderView::section:first {
            border-top-left-radius: 6px;
        }
        QHeaderView::section:last {
            border-top-right-radius: 6px;
            border-right: none;
        }
        /* 滚动条 */
        QScrollBar:vertical {
            background: #0A0E14;
            width: 6px;
            border-radius: 3px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background: #21262D;
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #30363D;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: #0A0E14;
            height: 6px;
            border-radius: 3px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal {
            background: #21262D;
            border-radius: 3px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #30363D;
        }
        /* 分组框 */
        QGroupBox {
            font-weight: 600;
            font-size: 12px;
            font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Microsoft YaHei', sans-serif;
            color: #8B949E;
            border: 1px solid #21262D;
            border-radius: 6px;
            margin-top: 10px;
            padding: 8px 12px 8px 12px;
            background: transparent;
        }
        QGroupBox::title {
            color: #0066CC;
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 6px;
            font-weight: 600;
            font-size: 11.5px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        /* 工具提示 */
        QToolTip {
            background: #21262D;
            color: #E6EDF3;
            border: 1px solid #30363D;
            border-radius: 4px;
            padding: 5px 8px;
            font-size: 12px;
        }
        /* 消息框 */
        QMessageBox {
            background: #161B22;
        }
        QMessageBox QLabel {
            color: #E6EDF3;
        }
    )");
}

MainWindow::~MainWindow() { AuthService::instance().logout(); }

void MainWindow::setupMenuBar() {
    // === 文件菜单 ===
    QMenu* fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction("新建笔录", this, [=](){ onMenuNav("新建笔录"); }, QKeySequence(tr("Ctrl+N")));
    fileMenu->addAction("审批中心", this, [=](){ onMenuNav("审批中心"); }, QKeySequence(tr("Ctrl+P")));
    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction("退出", this, &MainWindow::onMenuExit);
    quitAction->setShortcut(QKeySequence::Quit);

    // === 系统管理菜单 ===
    if (AuthService::instance().isAdmin()) {
        QMenu* sysMenu = menuBar()->addMenu("系统管理");
        sysMenu->addAction("用户管理", this, [=](){ onMenuNav("用户管理"); });
        sysMenu->addAction("日志审计", this, [=](){ onMenuNav("日志审计"); });
        sysMenu->addAction("数据备份", this, [=](){ onMenuNav("数据备份"); });
        sysMenu->addSeparator();
        sysMenu->addAction("修改密码", this, &MainWindow::onChangePassword, QKeySequence(tr("Ctrl+B")));
    }

    // === 视图菜单 ===
    QMenu* viewMenu = menuBar()->addMenu("视图");
    viewMenu->addAction("刷新数据", this, &MainWindow::onRefreshData, QKeySequence(tr("F5")));
    viewMenu->addSeparator();
    // 窗口控制（GNOME 等环境若隐藏了标题栏按钮，可通过菜单操作）
    QAction* minimizeAction = viewMenu->addAction("最小化", this, [this](){ showMinimized(); });
    minimizeAction->setShortcut(QKeySequence(tr("Ctrl+M"))); // Ctrl+M 最小化
    QAction* maximizeAction = viewMenu->addAction("最大化", this, [this](){
        if (isMaximized()) { showNormal(); } else { showMaximized(); }
    });
    maximizeAction->setShortcut(QKeySequence(tr("Ctrl+Shift+M"))); // Ctrl+Shift+M 全屏/还原
    QAction* fullscreenAction = viewMenu->addAction("还原窗口", this, [this](){ showNormal(); });
    viewMenu->addSeparator();
    viewMenu->addAction("罪犯信息管理", this, [=](){ onMenuNav("罪犯信息管理"); });

    // === 帮助菜单 ===
    QMenu* helpMenu = menuBar()->addMenu("帮助");
    helpMenu->addAction("操作手册", this, &MainWindow::onShowManual);
    helpMenu->addAction("关于", this, &MainWindow::onShowAbout);
}

void MainWindow::setupNav() {
    m_nav = new QTreeWidget(this);
    m_nav->setHeaderHidden(true);
    m_nav->setFixedWidth(210);

    // 核心业务（堆栈索引 0-3）
    QTreeWidgetItem* g1 = new QTreeWidgetItem(m_nav, {"核心业务"});
    g1->setExpanded(true);
    new QTreeWidgetItem(g1, {"罪犯信息管理"});
    new QTreeWidgetItem(g1, {"笔录制作"});
    new QTreeWidgetItem(g1, {"审批中心"});
    new QTreeWidgetItem(g1, {"档案管理"});

    // 资源配置（堆栈索引 4-7）
    QTreeWidgetItem* g2 = new QTreeWidgetItem(m_nav, {"资源配置"});
    new QTreeWidgetItem(g2, {"统计分析"});
    new QTreeWidgetItem(g2, {"模板管理"});
    new QTreeWidgetItem(g2, {"用户管理"});
    new QTreeWidgetItem(g2, {"数据备份"});

    if (AuthService::instance().isAdmin()) {
        QTreeWidgetItem* g3 = new QTreeWidgetItem(m_nav, {"系统管理"});
        new QTreeWidgetItem(g3, {"日志审计"});
    }

    // 显式映射：菜单名称 → 堆栈页面索引（直接查表，不再用累加偏移）
    m_navIndexMap = {
        {"罪犯信息管理", 0},
        {"笔录制作",    1},
        {"审批中心",    2},
        {"档案管理",    3},
        {"统计分析",    4},
        {"模板管理",    5},
        {"用户管理",    6},
        {"数据备份",    7},
        {"日志审计",    8},
    };

    connect(m_nav, &QTreeWidget::itemClicked, this, &MainWindow::onNavClicked);
}

void MainWindow::setupContent() {
    QWidget* central = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(central);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->addWidget(m_nav);
    QFrame* sep = new QFrame(central);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("QFrame{background:#21262D;}");
    layout->addWidget(sep);
    sep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_stack = new QStackedWidget(central);
    m_stack->addWidget(new CriminalListWidget(m_stack)); // 0 罪犯信息管理
    m_stack->addWidget(new RecordEditorWidget(m_stack)); // 1 笔录制作
    m_stack->addWidget(new ApprovalWidget(m_stack));      // 2 审批中心
    m_stack->addWidget(new ArchiveWidget(m_stack));       // 3 档案管理
    m_stack->addWidget(new StatisticsWidget(m_stack));    // 4 统计分析
    m_stack->addWidget(new TemplateWidget(m_stack));       // 5 模板管理
    m_stack->addWidget(new UserManageWidget(m_stack));     // 6 用户管理
    m_stack->addWidget(new BackupWidget(m_stack));         // 7 数据备份
    m_stack->addWidget(new LogViewerWidget(m_stack));      // 8 日志审计
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);
}

void MainWindow::setupStatusBar() {
    auto u = AuthService::instance().currentUser();
    m_userLabel = new QLabel(QString("当前用户：%1（%2）| %3 | %4")
        .arg(u.realName).arg(u.roleString()).arg(u.department)
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd")));
    statusBar()->addWidget(m_userLabel);

    // P2P 节点状态标签
    m_p2pLabel = new QLabel("P2P：未启动");
    m_p2pLabel->setStyleSheet("color:#8B949E;font-size:12px;padding:0 8px;");
    statusBar()->addPermanentWidget(m_p2pLabel);

    // 连接 P2P 服务信号
    auto& p2p = P2PNodeService::instance();
    connect(&p2p, &P2PNodeService::runningChanged, this, &MainWindow::onP2PRunningChanged);
    connect(&p2p, &P2PNodeService::peerOnline, this, &MainWindow::onPeerOnline);
    connect(&p2p, &P2PNodeService::peerOffline, this, &MainWindow::onPeerOffline);
    connect(&p2p, &P2PNodeService::scanIndexReceived, this, &MainWindow::onScanIndexReceived);

    // 同步当前 P2P 状态
    if (p2p.isRunning()) onP2PRunningChanged(true);

    statusBar()->addPermanentWidget(new QLabel("数据库：正常"));
    statusBar()->addPermanentWidget(new QLabel("今日笔录：--"));
}

void MainWindow::onNavClicked(QTreeWidgetItem* item, int) {
    if (!item->parent()) return;
    QTreeWidgetItem* parent = item->parent();
    int childIdx = parent->indexOfChild(item);
    int topIdx = m_nav->indexOfTopLevelItem(parent);
    int navIdx = 0;
    // 累加所有前面的顶级节点的子节点数量，得到正确偏移
    for (int i = 0; i < topIdx; ++i) navIdx += m_nav->topLevelItem(i)->childCount();
    navIdx += childIdx;
    if (navIdx >= 0 && navIdx < m_stack->count()) m_stack->setCurrentIndex(navIdx);
}

void MainWindow::onMenuExit() { close(); }

void MainWindow::onChangePassword() {
    QDialog dlg(this);
    dlg.setWindowTitle("修改密码");
    dlg.setFixedSize(400, 220);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    QFormLayout* f = new QFormLayout;
    QLineEdit* oldEdit = new QLineEdit;
    oldEdit->setEchoMode(QLineEdit::Password);
    QLineEdit* newEdit = new QLineEdit;
    newEdit->setEchoMode(QLineEdit::Password);
    QLineEdit* confirmEdit = new QLineEdit;
    confirmEdit->setEchoMode(QLineEdit::Password);
    f->addRow("原密码：", oldEdit);
    f->addRow("新密码：", newEdit);
    f->addRow("确认密码：", confirmEdit);
    v->addLayout(f);
    QDialogButtonBox* btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    v->addWidget(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;
    QString newPw = newEdit->text();
    QString confirmPw = confirmEdit->text();
    if (newPw.isEmpty() || newPw.length() < 6) {
        QMessageBox::warning(this, "错误", "新密码不能少于6位");
        return;
    }
    if (newPw != confirmPw) {
        QMessageBox::warning(this, "错误", "两次输入的密码不一致");
        return;
    }
    auto u = AuthService::instance().currentUser();
    bool ok = AuthService::instance().changePassword(u.id, oldEdit->text(), newPw);
    if (ok) {
        AuditService::instance().log(u.id, u.realName,
            AuditService::ACTION_UPDATE, "用户管理", "User", u.id, "修改了自己的密码");
        QMessageBox::information(this, "成功", "密码修改成功，请重新登录");
    } else {
        QMessageBox::warning(this, "失败", "密码修改失败");
    }
}

void MainWindow::onRefreshData() {
    QWidget* cur = m_stack->currentWidget();
    if (cur) cur->update();
    statusBar()->showMessage("数据已刷新", 2000);
    AuditService::instance().log(
        AuthService::instance().currentUser().id,
        AuthService::instance().currentUser().realName,
        AuditService::ACTION_VIEW, "视图", "MainWindow", 0, "刷新数据");
}

void MainWindow::onMenuNav(const QString& module) {
    QMap<QString, int> map = {
        {"罪犯信息管理", 0}, {"笔录制作", 1}, {"审批中心", 2},
        {"档案管理", 3}, {"模板管理", 4}, {"用户管理", 5},
        {"日志审计", 6}, {"数据备份", 7}, {"新建笔录", 1}
    };
    if (map.contains(module) && map[module] < m_stack->count()) {
        m_stack->setCurrentIndex(map[module]);
    }
}

void MainWindow::onShowAbout() {
    QDialog dlg(this);
    dlg.setWindowTitle("关于");
    dlg.setFixedSize(420, 280);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    QLabel* title = new QLabel("监狱审讯笔录工具");
    title->setStyleSheet("font-size:20px;font-weight:600;color:#0066CC;padding:8px;");
    title->setAlignment(Qt::AlignCenter);
    v->addWidget(title);
    QLabel* ver = new QLabel("版本 1.0.0");
    ver->setStyleSheet("font-size:13px;color:#555;");
    ver->setAlignment(Qt::AlignCenter);
    v->addWidget(ver);
    QLabel* desc = new QLabel(
        "<p style='line-height:1.8;'>专为监狱审讯场景打造的笔录管理工具<br/>"
        "技术栈：Qt6 + C++17 + SQLite<br/>"
        "开发单位：监狱信息科</p>"
    );
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("font-size:13px;color:#333;padding:8px;");
    v->addWidget(desc);
    QDialogButtonBox* btn = new QDialogButtonBox(QDialogButtonBox::Ok);
    btn->button(QDialogButtonBox::Ok)->setText("确定");
    connect(btn, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    v->addWidget(btn);
    dlg.exec();
}

void MainWindow::onShowManual() {
    QDialog dlg(this);
    dlg.setWindowTitle("操作手册");
    dlg.resize(680, 520);
    QVBoxLayout* v = new QVBoxLayout(&dlg);
    QTextBrowser* browser = new QTextBrowser;
    browser->setOpenExternalLinks(true);
    browser->setHtml(R"(
<h2 style='color:#0066CC;'>监狱审讯笔录工具 操作手册</h2>
<h3 style='color:#2E5077;'>一、系统登录</h3>
<p>使用管理员分配的用户名和密码登录系统。普通用户和管理员的操作权限不同。</p>
<h3 style='color:#2E5077;'>二、罪犯信息管理</h3>
<p>点击左侧「罪犯信息管理」，可查看、搜索、新增、编辑罪犯档案。双击列表行可查看详情并关联笔录。</p>
<h3 style='color:#2E5077;'>三、笔录制作</h3>
<p>新建笔录需选择关联的罪犯，填写审讯信息（时间、地点、类型等），笔录内容支持富文本编辑和模板填充。完成后提交审批。</p>
<h3 style='color:#2E5077;'>四、审批中心（管理员）</h3>
<p>管理员可查看所有待审批笔录，可通过或驳回。被驳回笔录可由记录员重新修改后再次提交。</p>
<h3 style='color:#2E5077;'>五、档案管理</h3>
<p>档案封存后罪犯信息不可直接编辑。如需解封请联系管理员操作。</p>
<h3 style='color:#2E5077;'>六、模板管理（管理员）</h3>
<p>管理员可新增、编辑笔录模板，供记录员在制作笔录时选择使用。</p>
<h3 style='color:#2E5077;'>七、数据备份（管理员）</h3>
<p>建议定期进行数据库备份。可从备份历史中还原任意历史版本。</p>
<h3 style='color:#2E5077;'>八、日志审计（管理员）</h3>
<p>记录所有用户的操作行为，支持按操作人、类型、模块、关键词和时间范围查询。可导出CSV。</p>
<h3 style='color:#2E5077;'>快捷键说明</h3>
<table width='100%' border='1' cellpadding='6' style='border-collapse:collapse;font-size:13px;'>
<tr style='background:#E8F0FE;'><td>Ctrl+N</td><td>新建笔录</td></tr>
<tr><td>Ctrl+B</td><td>修改密码</td></tr>
<tr style='background:#E8F0FE;'><td>Ctrl+P</td><td>审批中心</td></tr>
<tr><td>F5</td><td>刷新当前数据</td></tr>
<tr style='background:#E8F0FE;'><td>Ctrl+Q</td><td>退出系统</td></tr>
</table>
)");
    v->addWidget(browser);
    QPushButton* closeBtn = new QPushButton("关闭");
    closeBtn->setStyleSheet("QPushButton{padding:6px 24px;background:#0066CC;color:#FFF;border-radius:4px;font-weight:600;}"
        "QPushButton:hover{background:#0052A3;}");
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    v->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::updateStatusTime() {
    if (m_timeLabel) {
        m_timeLabel->setText("  " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    }
    auto u = AuthService::instance().currentUser();
    if (m_userLabel) {
        m_userLabel->setText(QString("当前用户：%1（%2）| %3 | %4")
            .arg(u.realName).arg(u.roleString()).arg(u.department)
            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd")));
    }
}

void MainWindow::onP2PRunningChanged(bool running) {
    if (running) {
        m_p2pLabel->setText(QString("P2P：在线 | 节点 %1 个").arg(m_peerCount));
        m_p2pLabel->setStyleSheet("color:#22C55E;font-size:12px;padding:0 8px;");
    } else {
        m_p2pLabel->setText("P2P：未启动");
        m_p2pLabel->setStyleSheet("color:#8B949E;font-size:12px;padding:0 8px;");
    }
}

void MainWindow::onPeerOnline(const QString& nodeId, const QHostAddress& addr) {
    Q_UNUSED(addr);
    ++m_peerCount;
    m_p2pLabel->setText(QString("P2P：在线 | 节点 %1 个").arg(m_peerCount));
    m_p2pLabel->setStyleSheet("color:#27AE60;font-size:12px;padding:0 8px;");
    statusBar()->showMessage(QString("[P2P] 新节点加入：%1 @ %2")
        .arg(nodeId).arg(addr.toString()), 4000);
}

void MainWindow::onPeerOffline(const QString& nodeId) {
    Q_UNUSED(nodeId);
    --m_peerCount;
    if (m_peerCount < 0) m_peerCount = 0;
    m_p2pLabel->setText(QString("P2P：在线 | 节点 %1 个").arg(m_peerCount));
    if (m_peerCount == 0) {
        m_p2pLabel->setText("P2P：在线（无其他节点）");
    }
    statusBar()->showMessage(QString("[P2P] 节点离线：%1").arg(nodeId), 3000);
}

void MainWindow::onScanIndexReceived(const QString& recordId, const QString& authorNodeId,
                                     qint64 fileSize, const QString& fileCID) {
    Q_UNUSED(fileSize);
    Q_UNUSED(fileCID);
    // 通知用户：有新的扫描件索引从网络到来（来自其他节点的广播）
    statusBar()->showMessage(QString("[P2P] 收到扫描索引：笔录 %1（来自节点 %2）—— 可在档案管理中查看")
        .arg(recordId).arg(authorNodeId), 5000);
    qDebug() << "[P2P-UI] 扫描索引通知：" << recordId << "作者节点：" << authorNodeId;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // 1. 停止 P2P 节点服务（关闭 UDP/TCP 端口，停止广播）
    if (P2PNodeService::instance().isRunning()) {
        qDebug() << "[MainWindow] 关闭中：停止 P2P 节点服务";
        P2PNodeService::instance().stop();
    }

    // 2. 锁定 KeyManager（清零内存中的主密钥）
    if (KeyManager::instance().isUnlocked()) {
        qDebug() << "[MainWindow] 关闭中：锁定 KeyManager，清除内存主密钥";
        KeyManager::instance().lock();
    }

    event->accept();
}
