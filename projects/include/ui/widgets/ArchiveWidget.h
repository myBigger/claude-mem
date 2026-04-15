#ifndef ARCHIVEWIDGET_H
#define ARCHIVEWIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QLineEdit>
#include <QTextEdit>
class ArchiveWidget : public QWidget { Q_OBJECT
public:
    explicit ArchiveWidget(QWidget* parent=nullptr);
private slots:
    void loadArchives();
    void onSearch();
    void onExportPdf();
    void onBatchExport();
    void onPreview(QTableWidgetItem* item);
    void onStatistics();
private:
    void setupUI();
    void exportSinglePdf(int recordId);
    QTableWidget* m_table = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QDateEdit* m_startDateEdit = nullptr;
    QDateEdit* m_endDateEdit = nullptr;
    QTextEdit* m_preview = nullptr;
    int m_currentRecordId = 0;
};
#endif