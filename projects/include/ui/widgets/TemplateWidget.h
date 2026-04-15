#ifndef TEMPLATEWIDGET_H
#define TEMPLATEWIDGET_H
#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
class TemplateWidget : public QWidget { Q_OBJECT
public:
    explicit TemplateWidget(QWidget* parent=nullptr);
private slots:
    void loadTemplates();
    void onNewTemplate();
    void onEditTemplate(const QModelIndex&);
    void onToggleStatus(int row, int col);
    void onPreview();
    void onDelete();
    void onImport();
    void onExport();
private:
    void setupUI();
    class QTableWidget* m_table = nullptr;
    class QTextEdit* m_preview = nullptr;
};
#endif