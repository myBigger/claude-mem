#ifndef RECORDEDITORWIDGET_H
#define RECORDEDITORWIDGET_H
#include <QWidget>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>
#include <QTableWidget>
#include <QTimer>
class LinedTextEdit;
class RecordEditorWidget : public QWidget { Q_OBJECT
public:
    explicit RecordEditorWidget(QWidget* parent = nullptr);
private slots:
    void onNewRecord();
    void onSelectCriminal();
    void onSaveDraft();
    void onSubmit();
    void onLoadTemplate(int typeIndex);
    void onAutoSave();
    void loadDrafts();
    void onLoadDraft();
    void onPrintRecord();
private:
    void setupUI();
    QString renderTemplate(const QString& templateContent, int criminalId);
    QString collectFormData();
    bool validateRecord();
    void showCriminalSelector();
    void updateScanButtonState();

    class LinedTextEdit* m_editor = nullptr;
    class RecordThemeSelector* m_themeSelector = nullptr;
    class QPushButton* m_uploadScanBtn = nullptr;
    class QPushButton* m_downloadScanBtn = nullptr;
    class QComboBox* m_typeCombo = nullptr;
    class QComboBox* m_criminalCombo = nullptr;
    class QLabel* m_criminalNameLabel = nullptr;
    class QLineEdit* m_recordIdEdit = nullptr;
    class QDateTimeEdit* m_datetimeEdit = nullptr;
    class QComboBox* m_locationCombo = nullptr;
    class QLineEdit* m_presentPersonsEdit = nullptr;
    class QTableWidget* m_draftsTable = nullptr;
    class QLabel* m_statusLabel = nullptr;
    class QLabel* m_saveTimeLabel = nullptr;
    int m_currentCriminalId = 0;
    int m_currentTemplateId = 0;
    int m_currentRecordId = 0;
    QString m_currentCriminalName;
    bool m_isDirty = false;
    QTimer* m_autoSaveTimer = nullptr;
};
#endif