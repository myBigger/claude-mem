#ifndef USERMANAGEWIDGET_H
#define USERMANAGEWIDGET_H
#include <QWidget>
#include <QTableWidget>
class UserManageWidget : public QWidget { Q_OBJECT
public:
    explicit UserManageWidget(QWidget* parent=nullptr);
private slots:
    void loadUsers();
    void onAdd();
    void onEdit();
    void onResetPassword();
    void onToggleEnabled();
private:
    void setupUI();
    class QTableWidget* m_table = nullptr;
};
#endif