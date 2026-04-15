#ifndef CRIMINALDETAILWIDGET_H
#define CRIMINALDETAILWIDGET_H
#include <QDialog>
#include <QTableWidget>
class CriminalDetailWidget : public QDialog {
    Q_OBJECT
public:
    explicit CriminalDetailWidget(const QString& cid, QWidget* parent = nullptr);
    ~CriminalDetailWidget();
private slots:
    void onNewRecord();
    void onPrintRecord();
    void onViewRecordDetail(int row, int col);
    void onRefreshRecords();
private:
    void loadData();
    QString m_cid;
    int m_criminalDbId = 0;
    QTableWidget* m_recordsTable = nullptr;
    QMap<int, int> m_recordRowToId;
};
#endif
