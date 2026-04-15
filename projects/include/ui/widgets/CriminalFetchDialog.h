#ifndef CRIMINALFETCHDIALOG_H
#define CRIMINALFETCHDIALOG_H
#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QJsonArray>
#include <QMap>
class CriminalFetchDialog : public QDialog {
    Q_OBJECT
public:
    explicit CriminalFetchDialog(QWidget* parent = nullptr);
    ~CriminalFetchDialog();
    QStringList selectedCriminalIds() const;
private slots:
    void onLogin();
    void onSearchById();
    void onSearchByName();
    void onApiSuccess(const QJsonArray& results);
    void onApiError(const QString& err);
    void onImportSelected();
    void onSelectAll();
    void onClearSelection();
    void onLoginSuccess();
    void onLoginFailed(const QString& err);
private:
    void setupUI();
    void populateTable(const QJsonArray& results);
    void setLoading(bool on);
    void updateImportBtnText();
    void showLoginPanel();
    void showSearchPanel();

    QWidget* m_loginPanel = nullptr;
    QWidget* m_searchPanel = nullptr;
    QLineEdit* m_casUrlEdit = nullptr;
    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QPushButton* m_loginBtn = nullptr;
    QLabel* m_loginStatusLabel = nullptr;
    QLineEdit* m_idEdit = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_importBtn = nullptr;
    QPushButton* m_searchIdBtn = nullptr;
    QPushButton* m_searchNameBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QJsonArray m_currentResults;
};
#endif // CRIMINALFETCHDIALOG_H
