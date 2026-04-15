#ifndef SCANUPLOADDlg_H
#define SCANUPLOADDlg_H

#include <QDialog>
#include <QLabel>
#include <QByteArray>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include "services/P2PNodeService.h"
#include "services/CryptoService.h"

/**
 * @brief 扫描件上传对话框
 *
 * 流程：选择扫描件 PDF → 显示文件信息 → 输入 PIN → 加密存储 + SHA-256 + 签名
 *
 * 用户首次使用时会自动生成 RSA-2048 密钥对。
 * 私钥由用户 PIN 加密保护（AES-256），存储在本地。
 */
class ScanUploadDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param recordId        当前笔录的 record_id（字符串）
     * @param recordDbId       当前笔录的数据库主键 ID
     * @param parent           父窗口
     */
    explicit ScanUploadDialog(const QString& recordId, int recordDbId,
                              QWidget* parent = nullptr);
    ~ScanUploadDialog() override;

private slots:
    void onSelectFile();
    void onUpload();
    void onGenerateKeys();

private:
    void setupUI();
    void loadOrGenerateKeys();
    bool decryptPrivateKey(const QString& pin);
    QString encryptAndSaveFile(const QString& srcPath, const QString& recordId);
    QString computeFileHash(const QString& filePath);
    QString signHash(const QString& hashBase64);
    bool saveScanRecord(const QString& recordId, int dbId,
                        const QString& hash, const QString& signature,
                        const QString& encryptedPath, qint64 fileSize);
    void setStep(int step, const QString& label, bool ok);

    // UI 元素
    QLabel*       m_filePathLabel = nullptr;
    QLabel*       m_fileSizeLabel = nullptr;
    QLabel*       m_hashLabel     = nullptr;
    QLabel*       m_statusLabel   = nullptr;
    QLabel*       m_pinStatusLabel = nullptr;
    QLineEdit*    m_pinEdit       = nullptr;
    QPushButton*  m_selectBtn     = nullptr;
    QPushButton*  m_uploadBtn     = nullptr;
    QPushButton*  m_genKeysBtn    = nullptr;
    QPushButton*  m_cancelBtn     = nullptr;
    QTextEdit*    m_logEdit       = nullptr;

    // 状态
    QString       m_selectedFile;
    QString       m_recordId;
    int           m_recordDbId = 0;
    bool          m_keysExist = false;
    QByteArray    m_publicKeyPEM;
    QByteArray    m_encryptedPrivateKey; // PIN 加密后的私钥存储（Base64）

    // 运行时私钥（解密后暂存，用完即清）
    QString       m_runtimePrivateKey;
};

#endif // SCANUPLOADDlg_H
