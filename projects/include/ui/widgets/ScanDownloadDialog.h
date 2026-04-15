#ifndef SCANDOWNLOADDIALOG_H
#define SCANDOWNLOADDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QByteArray>

class QLabel;
class QTableWidget;
class QLineEdit;
class QPushButton;
class QTextEdit;

/**
 * @brief 扫描件下载对话框 — 从 P2P 网络下载并解密扫描件
 *
 * 功能：
 * - 显示当前 P2P 网络中各节点持有的扫描件索引
 * - 从指定节点拉取加密扫描件
 * - 用用户私钥（PIN 保护）解密扫描件
 * - 导出解密后的 PDF 文件
 *
 * 使用场景：
 * - 用户在档案管理中找不到本地扫描件，可从其他工作站节点下载
 * - 扫描件全程以密文传输，只有持有私钥者能解密
 */
class ScanDownloadDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScanDownloadDialog(QWidget* parent = nullptr);
    ~ScanDownloadDialog() override;

private slots:
    void onRefresh();
    void onDownload();
    void onExportPdf();
    void onPeerScanIndexReceived(const QString& recordId, const QString& authorNodeId,
                                  qint64 fileSize, const QString& fileCID);

private:
    void setupUI();
    void loadScanList();
    void refreshScanList();
    bool decryptScanData(const QByteArray& encryptedData, QByteArray& outPlain);
    bool decryptPrivateKey(const QString& pin);

    // UI
    QLabel*       m_networkStatusLabel = nullptr;
    QTableWidget* m_scanTable = nullptr;
    QTextEdit*    m_logEdit = nullptr;
    QLineEdit*    m_pinEdit = nullptr;
    QLabel*       m_pinStatusLabel = nullptr;
    QPushButton*  m_downloadBtn = nullptr;
    QPushButton*  m_exportBtn = nullptr;

    // 数据
    QString       m_selectedRecordId;
    QByteArray    m_downloadedData;   // 解密后的原始 PDF 数据
    QByteArray    m_encryptedPrivateKey;
    QString       m_runtimePrivateKey;

    // P2P 网络已知扫描索引
    QMap<QString, QStringList> m_scanPeersMap; // recordId → peerNodeId列表
};

#endif // SCANDOWNLOADDIALOG_H
