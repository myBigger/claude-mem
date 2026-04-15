#ifndef PINDIALOG_H
#define PINDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

/**
 * PIN 设置/解锁对话框
 *
 * 首次使用（needsSetup == true）：
 *   - 要求用户设置 6 位数字 PIN
 *   - 生成主密钥并用 PIN 加密存储
 *   - 解锁后程序继续启动
 *
 * 已有密钥（needsSetup == false）：
 *   - 要求用户输入 PIN 解锁
 *   - PIN 错误可重试，3次错误弹警告
 *   - 解锁成功后程序继续启动
 */
class PINDialog : public QDialog {
    Q_OBJECT
public:
    // 首次设置模式
    explicit PINDialog(QWidget* parent = nullptr);
    // 解锁模式
    explicit PINDialog(bool unlockOnly, QWidget* parent = nullptr);
    ~PINDialog() override;

    bool isUnlocked() const { return m_unlocked; }

private slots:
    void onSubmit();

private:
    void setupSetupMode();
    void setupUnlockMode();
    bool m_unlockOnly = false;
    bool m_unlocked = false;
    int m_attempts = 0;

    QLineEdit* m_pinEdit = nullptr;
    QLineEdit* m_confirmEdit = nullptr;
    QLabel* m_msgLabel = nullptr;
    QPushButton* m_submitBtn = nullptr;
};

#endif // PINDIALOG_H
