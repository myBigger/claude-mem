#ifndef BACKUPWIDGET_H
#define BACKUPWIDGET_H
#include <QWidget>
class QLabel;
class QTableWidget;
class BackupWidget : public QWidget {
    Q_OBJECT
public:
    explicit BackupWidget(QWidget* parent=nullptr);
private slots:
    void onBackup();
    void onRestore();
    void onRestoreHistory(int row, int col);
    void onExportBackup();
    void loadHistory();
private:
    QLabel* m_dbPathLabel = nullptr;
    QLabel* m_dbSizeLabel = nullptr;
    QLabel* m_lastBackupLabel = nullptr;
    QTableWidget* m_historyTable = nullptr;
};
#endif
