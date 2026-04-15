#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMap>
#include <QHostAddress>
class QTreeWidget;
class QStackedWidget;
class QLabel;
class QTreeWidgetItem;
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void onNavClicked(QTreeWidgetItem* item, int col);
    void onMenuExit();
    void onChangePassword();
    void onRefreshData();
    void onMenuNav(const QString& module);
    void onShowAbout();
    void onShowManual();
    void updateStatusTime();
    void onP2PRunningChanged(bool running);
    void onPeerOnline(const QString& nodeId, const QHostAddress& addr);
    void onPeerOffline(const QString& nodeId);
    void onScanIndexReceived(const QString& recordId, const QString& authorNodeId,
                             qint64 fileSize, const QString& fileCID);
private:
    void setupMenuBar();
    void setupNav();
    void setupContent();
    void setupStatusBar();
protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QTreeWidget* m_nav = nullptr;
    QStackedWidget* m_stack = nullptr;
    QLabel* m_userLabel = nullptr;
    QLabel* m_timeLabel = nullptr;
    QLabel* m_p2pLabel = nullptr;
    int m_peerCount = 0;
    QMap<QString, int> m_navIndexMap;
};
#endif
