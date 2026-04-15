#include "ui/widgets/StatisticsWidget.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QFrame>
#include <QPushButton>
#include <QDateTime>
#include <QDebug>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>

StatisticsWidget::StatisticsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadStatistics();
}

StatisticsWidget::~StatisticsWidget() = default;

void StatisticsWidget::setupUI()
{
    setStyleSheet(R"(
        QWidget{background:#0A0E14;}
        QLabel{color:#E6EDF3;font-family:'Inter','SF Pro','Segoe UI','Microsoft YaHei',sans-serif;}
        QPushButton{background:#1C2128;color:#E6EDF3;border:1px solid #21262D;border-radius:4px;padding:7px 16px;font-family:'Inter','SF Pro','Segoe UI','Microsoft YaHei',sans-serif;}
        QPushButton:hover{background:#30363D;border-color:#30363D;color:#FFFFFF;}
        QTableWidget{background:#0A0E14;color:#E6EDF3;gridline-color:#21262D;border:1px solid #21262D;border-radius:6px;}
        QTableWidget::item{color:#E6EDF3;padding:6px 8px;}
        QTableWidget::item:hover{background:#161B22;}
        QTableWidget::item:selected{background:#0066CC1A;color:#FFFFFF;font-weight:600;}
        QHeaderView::section{background:#161B22;color:#8B949E;font-weight:600;font-size:12px;padding:8px 10px;border-bottom:1px solid #21262D;border-right:1px solid #21262D;text-transform:uppercase;letter-spacing:0.5px;}
    )");
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(16);
    main->setContentsMargins(20, 20, 20, 20);

    // 标题栏
    QHBoxLayout* titleBar = new QHBoxLayout;
    QLabel* title = new QLabel("📊 数据统计中心");
    title->setStyleSheet("font-size:20px;font-weight:700;color:#FFFFFF;");
    titleBar->addWidget(title);
    titleBar->addStretch();

    QPushButton* refreshBtn = new QPushButton("🔄 刷新数据");
    titleBar->addWidget(refreshBtn);
    main->addLayout(titleBar);

    // 第一行：核心指标卡片
    QLabel* section1Label = new QLabel("核心指标");
    section1Label->setStyleSheet("font-size:12px;font-weight:600;color:#0066CC;margin-top:8px;text-transform:uppercase;letter-spacing:0.8px;");
    main->addWidget(section1Label);

    QGridLayout* cardGrid = new QGridLayout;
    cardGrid->setSpacing(16);

    // 创建统计卡片
    struct CardConfig {
        QString id;
        QString icon;
        QString label;
        QString color;
    };

    QList<CardConfig> cards = {
        {"total_records", "📝", "笔录总数", "#0066CC"},
        {"draft_records", "📋", "草稿笔录", "#F59E0B"},
        {"pending_approval", "⏳", "待审批", "#8B5CF6"},
        {"approved_month", "✅", "本月已通过", "#22C55E"},
        {"rejected_month", "❌", "本月已驳回", "#E53E3E"},
        {"total_criminals", "👥", "罪犯总数", "#14B8A6"},
        {"total_cases", "📁", "案件总数", "#3B82F6"},
        {"signed_scans", "🔐", "已签名扫描件", "#22C55E"},
    };

    for (int i = 0; i < cards.size(); ++i) {
        const CardConfig& cfg = cards[i];
        QFrame* card = new QFrame;
        card->setFrameShape(QFrame::Box);
        card->setStyleSheet(QString(
            "QFrame{background:#161B22;border-left:4px solid %1;border-radius:6px;"
            "padding:12px;margin:2px;border:1px solid #21262D;}"
            "QFrame:hover{background:#1C2128;}"
        ).arg(cfg.color));

        QVBoxLayout* cl = new QVBoxLayout(card);
        cl->setContentsMargins(12, 10, 12, 10);
        cl->setSpacing(4);

        QLabel* iconLabel = new QLabel(cfg.icon);
        iconLabel->setStyleSheet("font-size:24px;");
        QLabel* numLabel = new QLabel("0");
        numLabel->setStyleSheet(QString(
            "font-size:32px;font-weight:bold;color:#FFFFFF;"));
        numLabel->setObjectName(cfg.id + "_value");
        QLabel* lbl = new QLabel(cfg.label);
        lbl->setStyleSheet("font-size:12.5px;color:#8B949E;margin-top:4px;");

        cl->addWidget(iconLabel);
        cl->addWidget(numLabel);
        cl->addWidget(lbl);
        cl->addStretch();

        int row = i / 4;
        int col = i % 4;
        cardGrid->addWidget(card, row, col);

        CardWidgets cw;
        cw.frame = card;
        cw.valueLabel = numLabel;
        m_cards[cfg.id] = cw;
    }
    main->addLayout(cardGrid);

    // 第二行：近期活动表格
    QLabel* section2Label = new QLabel("📈 本月笔录趋势");
    section2Label->setStyleSheet("font-size:12px;font-weight:600;color:#0066CC;margin-top:12px;text-transform:uppercase;letter-spacing:0.8px;");
    main->addWidget(section2Label);

    QFrame* trendFrame = new QFrame;
    trendFrame->setFrameShape(QFrame::Box);
    trendFrame->setStyleSheet("QFrame{background:#161B22;border-radius:6px;padding:12px;border:1px solid #21262D;}");
    QVBoxLayout* trendLayout = new QVBoxLayout(trendFrame);

    QTableWidget* trendTable = new QTableWidget;
    trendTable->setColumnCount(5);
    trendTable->setHorizontalHeaderLabels({"笔录类型", "本月数量", "上月数量", "环比变化", "占比"});
    trendTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    trendTable->verticalHeader()->setVisible(false);
    trendTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    trendTable->setAlternatingRowColors(true);
    trendTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 填充趋势数据
    QStringList recordTypes = {"RT-01 入监笔录", "RT-02 日常谈话", "RT-03 案件调查",
                               "RT-04 行政处罚告知", "RT-05 减刑假释评估"};
    trendTable->setRowCount(recordTypes.size());

    for (int i = 0; i < recordTypes.size(); ++i) {
        int thisMonth = (i + 1) * 3;  // 模拟数据
        int lastMonth = (i + 1) * 2;
        int change = thisMonth - lastMonth;
        double ratio = thisMonth / 50.0 * 100;

        trendTable->setItem(i, 0, new QTableWidgetItem(recordTypes[i]));
        trendTable->setItem(i, 1, new QTableWidgetItem(QString::number(thisMonth)));
        trendTable->setItem(i, 2, new QTableWidgetItem(QString::number(lastMonth)));
        QTableWidgetItem* changeItem = new QTableWidgetItem(
            change >= 0 ? QString("📈 +%1").arg(change) : QString("📉 %1").arg(change));
        changeItem->setForeground(QBrush(change >= 0 ? QColor("#22C55E") : QColor("#E53E3E")));
        trendTable->setItem(i, 3, changeItem);
        trendTable->setItem(i, 4, new QTableWidgetItem(QString("%1%").arg(ratio, 0, 'f', 1)));
    }

    trendLayout->addWidget(trendTable);
    main->addWidget(trendFrame, 1);

    // 第三行：快捷操作
    QLabel* section3Label = new QLabel("⚡ 快捷操作");
    section3Label->setStyleSheet("font-size:12px;font-weight:600;color:#0066CC;margin-top:12px;text-transform:uppercase;letter-spacing:0.8px;");
    main->addWidget(section3Label);

    QHBoxLayout* quickActions = new QHBoxLayout;
    QStringList actions[] = {
        {"➕ 新建笔录", "#0066CC"},
        {"📋 笔录审批", "#8B5CF6"},
        {"🔍 档案查询", "#14B8A6"},
        {"📊 导出报表", "#F59E0B"}
    };

    for (const auto& action : actions) {
        QPushButton* btn = new QPushButton(action[0]);
        btn->setStyleSheet(QString(
            "QPushButton{background:%1;color:#FFF;padding:10px 20px;border-radius:4px;"
            "font-weight:bold;}"
            "QPushButton:hover{opacity:0.9;}").arg(action[1]));
        quickActions->addWidget(btn);
    }
    quickActions->addStretch();
    main->addLayout(quickActions);

    // 底部信息
    QLabel* footer = new QLabel(QString("数据更新时间: %1 | PrisonSIS v1.0.0")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    footer->setStyleSheet("color:#6E7681;font-size:12px;margin-top:12px;");
    footer->setAlignment(Qt::AlignRight);
    main->addWidget(footer);

    // 信号连接
    connect(refreshBtn, &QPushButton::clicked, this, &StatisticsWidget::refreshStatistics);
}

void StatisticsWidget::loadStatistics()
{
    StatisticsData data = fetchData();

    // 更新卡片
    updateCard("total_records", data.totalRecords, "#0066CC");
    updateCard("draft_records", data.draftRecords, "#F59E0B");
    updateCard("pending_approval", data.pendingApproval, "#8B5CF6");
    updateCard("approved_month", data.approvedThisMonth, "#22C55E");
    updateCard("rejected_month", data.rejectedThisMonth, "#E53E3E");
    updateCard("total_criminals", data.totalCriminals, "#14B8A6");
    updateCard("total_cases", data.totalCases, "#3B82F6");
    updateCard("signed_scans", data.signedScans, "#22C55E");
}

void StatisticsWidget::refreshStatistics()
{
    qDebug() << "[StatisticsWidget] 刷新统计数据...";
    loadStatistics();
}

StatisticsData StatisticsWidget::fetchData() const
{
    StatisticsData data = {0, 0, 0, 0, 0, 0, 0, 0};

    // 获取当前月份的第一天
    QString currentMonth = QDate::currentDate().toString("yyyy-MM");
    QString lastMonth = QDate::currentDate().addMonths(-1).toString("yyyy-MM");

    // 笔录总数
    auto totalResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records", {});
    data.totalRecords = totalResult.value("cnt").toInt();

    // 草稿笔录
    auto draftResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records WHERE status='Draft'", {});
    data.draftRecords = draftResult.value("cnt").toInt();

    // 待审批笔录
    auto pendingResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records WHERE status='PendingApproval'", {});
    data.pendingApproval = pendingResult.value("cnt").toInt();

    // 本月已通过
    auto approvedResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records WHERE status='Approved' AND updated_at LIKE :month",
        {{"month", currentMonth + "%"}});
    data.approvedThisMonth = approvedResult.value("cnt").toInt();

    // 本月已驳回
    auto rejectedResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records WHERE status='Rejected' AND updated_at LIKE :month",
        {{"month", currentMonth + "%"}});
    data.rejectedThisMonth = rejectedResult.value("cnt").toInt();

    // 罪犯总数
    auto criminalsResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM criminals", {});
    data.totalCriminals = criminalsResult.value("cnt").toInt();

    // 案件总数
    auto casesResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM cases", {});
    data.totalCases = casesResult.value("cnt").toInt();

    // 已签名扫描件
    auto scansResult = DatabaseManager::instance().selectOne(
        "SELECT COUNT(*) as cnt FROM records WHERE scan_signature IS NOT NULL AND scan_signature != ''", {});
    data.signedScans = scansResult.value("cnt").toInt();

    qDebug() << "[StatisticsWidget] 统计数据:" << data.totalRecords << data.pendingApproval;
    return data;
}

void StatisticsWidget::updateCard(const QString& cardId, int value, const QString& color)
{
    if (m_cards.contains(cardId)) {
        m_cards[cardId].valueLabel->setText(QString::number(value));
        m_cards[cardId].valueLabel->setStyleSheet(
            "font-size:32px;font-weight:bold;color:#FFFFFF;");
    }
}
