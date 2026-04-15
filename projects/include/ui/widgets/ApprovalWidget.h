#ifndef APPROVALWIDGET_H
#define APPROVALWIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QVector>
#include <QMap>
#include <QVariant>
class ApprovalWidget : public QWidget { Q_OBJECT
public:
    explicit ApprovalWidget(QWidget* parent=nullptr);
private slots:
    void loadApprovals();
    void onTabChanged(int index);
    void onApprove();
    void onReject();
    void onPreviewRecord(QTableWidgetItem* item);
private:
    void setupUI();
    void populateTable(const QVector<QMap<QString,QVariant>>& records);
    class QTabWidget* m_tabs = nullptr;
    class QTableWidget* m_pendingTable = nullptr;
    class QTableWidget* m_inApprovalTable = nullptr;
    class QTableWidget* m_approvedTable = nullptr;
    class QTableWidget* m_rejectedTable = nullptr;
    class QTextEdit* m_preview = nullptr;
    int m_currentRecordId = 0;
};
#endif