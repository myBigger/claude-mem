#ifndef LOGVIEWERWIDGET_H
#define LOGVIEWERWIDGET_H
#include <QWidget>
class QTableWidget;
class QLineEdit;
class QComboBox;
class QDateEdit;
class QLabel;
class LogViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogViewerWidget(QWidget* parent=nullptr);
private slots:
    void onSearch();
    void onExportCsv();
    void onClearLogs();
    void onRefresh();
    void onRowDoubleClicked(int row, int col);
    void loadLogs(int page=1);
private:
    int m_currentPage = 1;
    int m_pageSize = 50;
    int m_totalCount = 0;
    class QTableWidget* m_table = nullptr;
    class QLineEdit* m_userEdit = nullptr;
    class QComboBox* m_actionCombo = nullptr;
    class QComboBox* m_moduleCombo = nullptr;
    class QDateEdit* m_startDate = nullptr;
    class QDateEdit* m_endDate = nullptr;
    class QLineEdit* m_keywordEdit = nullptr;
    class QLabel* m_pageLabel = nullptr;
};
#endif
