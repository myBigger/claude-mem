#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H
#include <QWidget>
#include <QMap>
#include <QString>
#include <QLabel>

/**
 * @brief 统计数据结构
 */
struct StatisticsData {
    int totalRecords;      // 笔录总数
    int draftRecords;      // 草稿数量
    int pendingApproval;    // 待审批
    int approvedThisMonth;  // 本月已通过
    int rejectedThisMonth;  // 本月已驳回
    int totalCriminals;     // 罪犯总数
    int totalCases;        // 案件总数
    int signedScans;       // 已签名扫描件
};

class StatisticsWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatisticsWidget(QWidget* parent = nullptr);
    ~StatisticsWidget();

public slots:
    void refreshStatistics();

private:
    void setupUI();
    void loadStatistics();
    StatisticsData fetchData() const;
    void updateCard(const QString& cardId, int value, const QString& color);

    // 统计数据缓存
    StatisticsData m_data;

    // UI 组件指针
    struct CardWidgets {
        QWidget* frame;
        QLabel* valueLabel;
    };
    QMap<QString, CardWidgets> m_cards;
};

#endif // STATISTICSWIDGET_H
