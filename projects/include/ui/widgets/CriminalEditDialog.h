#ifndef CRIMINALEDITDIALOG_H
#define CRIMINALEDITDIALOG_H
#include <QDialog>
#include <QLineEdit>
#include <QString>
#include <QMap>
#include <QWidget>
#include <QVariant>
class CriminalEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit CriminalEditDialog(QWidget* parent=nullptr, const QString& cid="");
private slots:
    void onSave();
    void onIdCardChanged();
private:
    void setupUI();
    void loadExistingData();
    void populateFields(const QMap<QString,QVariant>& data);
    bool validate();
    QString m_cid;
    QMap<QString,QWidget*> m_fields; // fieldName -> widget
    QLineEdit* m_idEdit = nullptr;
};
#endif