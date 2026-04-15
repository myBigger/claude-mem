#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H
#include <QDialog>
class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    ~LoginDialog();
protected:
    void keyPressEvent(QKeyEvent* e) override;
private slots:
    void onLogin();
private:
    void setupUI();
    class Private; Private* d;
};
#endif
