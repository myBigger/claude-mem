#include "ui/widgets/ScanUploadDialog.h"
#include "services/P2PNodeService.h"
#include "services/CryptoService.h"
#include "services/AuthService.h"
#include "services/AuditService.h"
#include "database/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QFile>
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QMessageBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QFrame>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────
// 工具函数：PIN → AES 密钥（简单方案：PIN 做 salt，固定 SHA-256 派生）
// 注意：生产环境应使用 PBKDF2，此处用 SHA-256 简化
// ─────────────────────────────────────────────────────────────────
static QByteArray deriveKeyFromPin(const QString& pin)
{
    QByteArray salt = "PrisonSIS_KeyDerivation_v1";
    QByteArray input = (pin + QString::fromUtf8(salt)).toUtf8();
    return CryptoService::instance().sha256(input);
}

// ─────────────────────────────────────────────────────────────────
// ScanUploadDialog 实现
// ─────────────────────────────────────────────────────────────────
ScanUploadDialog::ScanUploadDialog(const QString& recordId, int recordDbId, QWidget* parent)
    : QDialog(parent)
    , m_recordId(recordId)
    , m_recordDbId(recordDbId)
{
    setWindowTitle("上传扫描件 — " + recordId);
    setMinimumSize(700, 520);
    resize(750, 560);
    setupUI();
    loadOrGenerateKeys();
}

ScanUploadDialog::~ScanUploadDialog() = default;

void ScanUploadDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog{background:#0A0E14;border:1px solid #21262D;border-radius:10px;}
        QLabel{color:#E6EDF3;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        #header{color:#E6EDF3;background:#161B22;border:1px solid #21262D;border-radius:8px;padding:10px;font-size:13px;}
        QGroupBox{font-weight:600;font-size:12px;color:#0066CC;border:1px solid #21262D;border-radius:8px;margin-top:10px;background:#0F1419;padding-top:10px;text-transform:uppercase;letter-spacing:0.5px;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;padding:0 8px;background:#0F1419;}
        QLineEdit{padding:8px 12px;border:1.5px solid #21262D;border-radius:4px;background:#0F1419;color:#E6EDF3;font-size:13px;}
        QLineEdit:focus{border-color:#0066CC;}
        QTextEdit{padding:6px 10px;border:1.5px solid #21262D;border-radius:4px;background:#0A0E14;color:#E6EDF3;font-family:'JetBrains Mono','SF Mono','Consolas',monospace;font-size:11px;}
        QTableWidget{background:#0A0E14;alternate-background-color:#0F1419;color:#E6EDF3;gridline-color:#21262D;border:1px solid #21262D;border-radius:6px;}
        QTableWidget::item{padding:6px 8px;}
        QTableWidget::item:selected{background:#0066CC1A;color:#FFFFFF;}
        QHeaderView::section{background:#161B22;color:#8B949E;font-weight:600;padding:6px;border-bottom:1px solid #21262D;border-right:1px solid #21262D;text-transform:uppercase;letter-spacing:0.5px;}
        QPushButton{background:#1C2128;color:#E6EDF3;border:none;border-radius:4px;padding:8px 18px;font-size:13px;font-family:'Microsoft YaHei',sans-serif;}
        QPushButton:hover{background:#30363D;}
        QPushButton:disabled{background:#0A0E14;color:#6E7681;}
        #uploadBtn{background:#0066CC;color:#FFFFFF;font-weight:600;border-radius:4px;}
        #uploadBtn:hover{background:#0052A3;}
        #uploadBtn:pressed{background:#003D82;}
        QScrollBar:vertical{background:#0A0E14;width:7px;border-radius:3px;}
        QScrollBar::handle:vertical{background:#21262D;border-radius:3px;min-height:24px;}
        QScrollBar::handle:vertical:hover{background:#30363D;}
    )");
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(12);

    // ── 顶部提示 ──
    QLabel* header = new QLabel(
        QString::fromUtf8("⚠ 请上传签字按手印后的纸质笔录扫描件（PDF 格式）。\n"
                           "上传后将计算数字指纹并添加您的数字签名，笔录将正式锁定，不可再修改。"));
    header->setObjectName("header");
    header->setWordWrap(true);
    main->addWidget(header);

    // ── 文件选择区 ──
    QGroupBox* fileBox = new QGroupBox("选择扫描件");
    QHBoxLayout* fileRow = new QHBoxLayout(fileBox);
    m_filePathLabel = new QLabel("尚未选择文件");
    m_filePathLabel->setStyleSheet("color:#8B949E;font-size:12px;");
    m_filePathLabel->setWordWrap(true);
    m_selectBtn = new QPushButton("选择 PDF 文件...");
    m_fileSizeLabel = new QLabel("");
    m_fileSizeLabel->setStyleSheet("color:#8B949E;font-size:12px;");
    fileRow->addWidget(m_filePathLabel, 1);
    fileRow->addWidget(m_fileSizeLabel);
    fileRow->addWidget(m_selectBtn);
    main->addWidget(fileBox);

    // ── PIN 验证区 ──
    QGroupBox* pinBox = new QGroupBox("身份验证（PIN 码）");
    QHBoxLayout* pinRow = new QHBoxLayout(pinBox);
    m_pinEdit = new QLineEdit;
    m_pinEdit->setPlaceholderText("输入您的 PIN 码（6位数字）...");
    m_pinEdit->setEchoMode(QLineEdit::Password);
    m_pinEdit->setMaximumWidth(200);
    m_pinStatusLabel = new QLabel("未验证");
    m_pinStatusLabel->setStyleSheet("color:#F59E0B;font-size:12px;");
    m_genKeysBtn = new QPushButton("首次使用需先生成密钥");
    m_genKeysBtn->setStyleSheet("color:#0066CC;font-size:12px;border:none;padding:0;background:transparent;text-decoration:underline;");
    pinRow->addWidget(new QLabel("PIN 码："));
    pinRow->addWidget(m_pinEdit, 1);
    pinRow->addWidget(m_pinStatusLabel);
    pinRow->addWidget(m_genKeysBtn);
    main->addWidget(pinBox);

    // ── 处理状态 ──
    QGroupBox* statusBox = new QGroupBox("处理状态");
    QFormLayout* statusForm = new QFormLayout(statusBox);
    m_hashLabel = new QLabel("--");
    m_hashLabel->setStyleSheet("font-family:'JetBrains Mono','SF Mono','Consolas',monospace;font-size:11px;color:#8B949E;word-break:break-all;");
    m_statusLabel = new QLabel("等待操作");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("font-size:12px;color:#8B949E;");
    statusForm->addRow("文件哈希 (SHA-256)：", m_hashLabel);
    statusForm->addRow("处理状态：", m_statusLabel);
    main->addWidget(statusBox, 1);

    // ── 操作日志 ──
    QLabel* logLabel = new QLabel("处理日志：");
    m_logEdit = new QTextEdit;
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(80);
    main->addWidget(logLabel);
    main->addWidget(m_logEdit);

    // ── 底部按钮 ──
    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_cancelBtn = new QPushButton("取消");
    m_cancelBtn->setMaximumWidth(100);
    m_uploadBtn = new QPushButton("确认上传并签名");
    m_uploadBtn->setObjectName("uploadBtn");
    m_uploadBtn->setEnabled(false);
    m_uploadBtn->setMaximumWidth(160);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_uploadBtn);
    main->addLayout(btnRow);

    // ── 信号连接 ──
    connect(m_selectBtn, &QPushButton::clicked, this, &ScanUploadDialog::onSelectFile);
    connect(m_genKeysBtn, &QPushButton::clicked, this, &ScanUploadDialog::onGenerateKeys);
    connect(m_pinEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_keysExist && text.length() >= 6) {
            // 自动尝试验证 PIN
            if (decryptPrivateKey(text)) {
                m_pinStatusLabel->setText("✓ PIN 验证成功");
                m_pinStatusLabel->setStyleSheet("color:#22C55E;font-size:12px;font-weight:600;");
                m_uploadBtn->setEnabled(!m_selectedFile.isEmpty());
            } else {
                m_pinStatusLabel->setText("✗ PIN 错误");
                m_pinStatusLabel->setStyleSheet("color:#E53E3E;font-size:12px;font-weight:600;");
                m_uploadBtn->setEnabled(false);
            }
        } else {
            m_pinStatusLabel->setText("未验证");
            m_pinStatusLabel->setStyleSheet("color:#F59E0B;font-size:12px;");
            m_uploadBtn->setEnabled(false);
        }
    });
    connect(m_uploadBtn, &QPushButton::clicked, this, &ScanUploadDialog::onUpload);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// ─────────────────────────────────────────────────────────────────
// 加载或生成密钥对
// ─────────────────────────────────────────────────────────────────
void ScanUploadDialog::loadOrGenerateKeys()
{
    auto& auth = AuthService::instance();
    QString uid = auth.currentUser().userId;
    QString keysDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/keys/" + uid;
    QDir().mkpath(keysDir);

    QString pubPath = keysDir + "/public_key.pem";
    QString privPath = keysDir + "/private_key.enc";

    if (QFile::exists(pubPath) && QFile::exists(privPath)) {
        // 加载公钥
        QFile f(pubPath);
        if (f.open(QIODevice::ReadOnly)) {
            m_publicKeyPEM = f.readAll();
            f.close();
        }
        // 加载加密私钥
        QFile g(privPath);
        if (g.open(QIODevice::ReadOnly)) {
            m_encryptedPrivateKey = g.readAll();
            g.close();
        }
        m_keysExist = !m_publicKeyPEM.isEmpty() && !m_encryptedPrivateKey.isEmpty();
        if (m_keysExist) {
            m_genKeysBtn->setVisible(false);
            m_pinStatusLabel->setText("密钥已就绪，请输入 PIN");
        }
    } else {
        m_keysExist = false;
        m_genKeysBtn->setVisible(true);
        m_pinStatusLabel->setText("尚未生成密钥");
        m_logEdit->append("[系统] 首次使用，需先生成密钥对。点击上方「首次使用需先生成密钥」。");
    }
}

// ─────────────────────────────────────────────────────────────────
// 生成 RSA-2048 密钥对（首次使用时调用）
// ─────────────────────────────────────────────────────────────────
void ScanUploadDialog::onGenerateKeys()
{
    QString pin = m_pinEdit->text();
    if (pin.length() < 6) {
        QMessageBox::warning(this, "提示", "请先输入至少6位 PIN 码，再点击生成密钥。");
        return;
    }
    m_logEdit->append("[密钥生成] 正在生成 RSA-2048 密钥对，请稍候...");
    m_genKeysBtn->setEnabled(false);
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QMap<QString, QString> keyPair;
    try {
        keyPair = CryptoService::instance().generateRSAKeyPair();
    } catch (const std::exception& e) {
        QApplication::restoreOverrideCursor();
        m_genKeysBtn->setEnabled(true);
        QMessageBox::critical(this, "错误", QString("密钥生成失败：%1").arg(e.what()));
        return;
    }
    QApplication::restoreOverrideCursor();

    if (keyPair.isEmpty()) {
        QMessageBox::critical(this, "错误", "密钥生成失败，请重试。");
        m_genKeysBtn->setEnabled(true);
        return;
    }

    m_publicKeyPEM = keyPair["publicKey"].toUtf8();
    QString privateKey = keyPair["privateKey"];

    // 用 PIN 派生的 AES 密钥加密私钥
    QByteArray aesKey = deriveKeyFromPin(pin);
    QByteArray iv;
    iv.resize(16);
    // 简单 IV：PIN 的 SHA-256 前16字节
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(pin.toUtf8());
    iv = hash.result().left(16);

    QByteArray encPriv = CryptoService::instance().encryptAES(
        privateKey.toUtf8(), aesKey, iv);

    if (encPriv.isEmpty()) {
        QMessageBox::critical(this, "错误", "私钥加密失败，请重试。");
        m_genKeysBtn->setEnabled(true);
        return;
    }

    // 保存
    auto& auth = AuthService::instance();
    QString uid = auth.currentUser().userId;
    QString keysDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/keys/" + uid;
    QDir().mkpath(keysDir);

    QFile pubFile(keysDir + "/public_key.pem");
    if (pubFile.open(QIODevice::WriteOnly)) {
        pubFile.write(m_publicKeyPEM);
        pubFile.close();
    }
    QFile privFile(keysDir + "/private_key.enc");
    if (privFile.open(QIODevice::WriteOnly)) {
        privFile.write(encPriv);
        privFile.close();
    }

    m_encryptedPrivateKey = encPriv;
    m_keysExist = true;
    m_genKeysBtn->setVisible(false);
    m_pinStatusLabel->setText("✓ 密钥已生成，请重新输入 PIN");
    m_pinStatusLabel->setStyleSheet("color:#a6e3a1;font-size:12px;font-weight:bold;");
    m_logEdit->append("[密钥生成] ✓ RSA-2048 密钥对已生成并安全存储。");
    m_logEdit->append("[提示] 请重新在 PIN 框输入您的 PIN 码以验证身份。");
    QMessageBox::information(this, "成功",
        "密钥已生成！\n\n请在 PIN 框重新输入您的 PIN 码，然后上传扫描件。\n\n"
        "⚠ 请务必记住您的 PIN 码，忘记将无法恢复！");
}

// ─────────────────────────────────────────────────────────────────
// 用 PIN 解密私钥
// ─────────────────────────────────────────────────────────────────
bool ScanUploadDialog::decryptPrivateKey(const QString& pin)
{
    if (m_encryptedPrivateKey.isEmpty()) return false;
    try {
        QByteArray aesKey = deriveKeyFromPin(pin);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(pin.toUtf8());
        QByteArray iv = hash.result().left(16);

        QByteArray dec = CryptoService::instance().decryptAES(
            m_encryptedPrivateKey, aesKey);
        if (dec.isEmpty()) return false;
        m_runtimePrivateKey = QString::fromUtf8(dec);
        return !m_runtimePrivateKey.isEmpty();
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────
// 选择文件
// ─────────────────────────────────────────────────────────────────
void ScanUploadDialog::onSelectFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "选择扫描件 PDF",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        "PDF 文件 (*.pdf *.PDF)");
    if (path.isEmpty()) return;

    QFileInfo info(path);
    if (info.size() == 0) {
        QMessageBox::warning(this, "错误", "文件为空，请重新选择。");
        return;
    }
    if (info.size() > 50 * 1024 * 1024) {
        QMessageBox::warning(this, "提示", "文件超过 50MB，请优化扫描分辨率后重试。");
        return;
    }

    m_selectedFile = path;
    m_filePathLabel->setText(path);
    m_filePathLabel->setStyleSheet("font-size:12px;color:#8B949E;");
    m_fileSizeLabel->setText(QString("  %1 KB").arg(info.size() / 1024));
    m_logEdit->append(QString("[文件] 已选择：%1").arg(path));

    // 自动计算哈希
    setStep(1, "计算文件哈希...", true);
    QString hashHex = computeFileHash(path);
    if (hashHex.isEmpty()) {
        setStep(1, "哈希计算失败", false);
        return;
    }
    m_hashLabel->setText(hashHex);
    m_hashLabel->setToolTip("SHA-256 哈希：此值将锁定笔录，任何篡改都能被发现");
    m_logEdit->append(QString("[哈希] SHA-256: %1").arg(hashHex));
    setStep(1, "文件已就绪，等待 PIN 验证", true);
    m_logEdit->append("[提示] 请在上方输入 PIN 码验证身份。");

    // 如果 PIN 已经验证过，自动启用上传按钮
    if (!m_runtimePrivateKey.isEmpty()) {
        m_uploadBtn->setEnabled(true);
    }
}

// ─────────────────────────────────────────────────────────────────
// 计算文件 SHA-256 哈希
// ─────────────────────────────────────────────────────────────────
QString ScanUploadDialog::computeFileHash(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return "";
    QByteArray data = f.readAll();
    f.close();
    QByteArray hash = CryptoService::instance().sha256(data);
    return hash.toHex();
}

// ─────────────────────────────────────────────────────────────────
// 加密并保存扫描件
// ─────────────────────────────────────────────────────────────────
QString ScanUploadDialog::encryptAndSaveFile(const QString& srcPath, const QString& recordId)
{
    // 生成随机 AES-256 密钥和 IV
    QByteArray aesKey, aesIV;
    CryptoService::instance().generateAESKey(aesKey, aesIV);

    // 读源文件
    QFile f(srcPath);
    if (!f.open(QIODevice::ReadOnly)) return "";
    QByteArray plainData = f.readAll();
    f.close();

    // AES 加密
    QByteArray encrypted = CryptoService::instance().encryptAES(plainData, aesKey, aesIV);
    if (encrypted.isEmpty()) return "";

    // 用 RSA 公钥加密 AES 密钥（附加在密文头部，以 Base64 存储）
    QByteArray encKey = CryptoService::instance().rsaEncrypt(
        aesKey, QString::fromUtf8(m_publicKeyPEM));
    if (encKey.isEmpty()) return "";

    // 存储格式：JSON { "encKey": "...", "encKeyIV": "...", "iv": "...", "cipher": "..." }
    QJsonObject obj;
    obj["encKey"]    = QString::fromUtf8(encKey.toBase64());
    obj["iv"]        = QString::fromUtf8(aesIV.toBase64());
    obj["cipher"]    = QString::fromUtf8(encrypted.toBase64());
    obj["originalSize"] = plainData.size();

    QByteArray jsonData = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    // 保存到 P2P 同步目录：AppData/p2p/scans/{nodeId}/
    // 节点 ID = SHA-256(公钥) 前16位，与 P2PNodeService 一致
    QString nodeId = QString::fromUtf8(
        CryptoService::instance().sha256(m_publicKeyPEM).toHex().left(16));
    QString scansDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                       + "/p2p/scans/" + nodeId;
    QDir().mkpath(scansDir);
    QString outPath = scansDir + "/" + recordId + "_scan.enc";
    QFile out(outPath);
    if (!out.open(QIODevice::WriteOnly)) return "";
    out.write(jsonData);
    out.close();

    m_logEdit->append(QString("[加密] ✓ 扫描件已 AES-256 加密存储：%1").arg(outPath));
    return outPath;
}

// ─────────────────────────────────────────────────────────────────
// RSA 签名哈希
// ─────────────────────────────────────────────────────────────────
QString ScanUploadDialog::signHash(const QString& hashBase64)
{
    QByteArray hashBytes = QByteArray::fromBase64(hashBase64.toUtf8());
    QByteArray sig = CryptoService::instance().signData(hashBytes, m_runtimePrivateKey);
    // 签名后清空私钥
    m_runtimePrivateKey.clear();
    if (sig.isEmpty()) return "";
    return QString::fromUtf8(sig.toBase64());
}

// ─────────────────────────────────────────────────────────────────
// 保存扫描件记录到数据库
// ─────────────────────────────────────────────────────────────────
bool ScanUploadDialog::saveScanRecord(const QString& recordId, int dbId,
                                      const QString& hash,
                                      const QString& signature,
                                      const QString& encryptedPath,
                                      qint64 fileSize)
{
    auto& auth = AuthService::instance();
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString hashHex = CryptoService::instance().sha256(
        QByteArray::fromBase64(hash.toUtf8())).toHex();

    QMap<QString, QVariant> vals = {
        {"scan_file_path", encryptedPath},
        {"scan_hash", hashHex},
        {"scan_signature", signature},
        {"scan_file_size", fileSize},
        {"scan_uploaded_at", now},
        {"scan_uploaded_by", auth.currentUser().id},
        {"status", "SignedScan"},      // 笔录正式锁定
        {"updated_at", now},
    };

    bool ok = DatabaseManager::instance().update("records", vals,
        "id=:id", {{"id", dbId}});

    if (ok) {
        AuditService::instance().log(auth.currentUser().id, auth.currentUser().realName,
            AuditService::ACTION_UPDATE, "笔录管理", "Record", dbId,
            "上传扫描件+数字签名:" + recordId);
    }
    return ok;
}

// ─────────────────────────────────────────────────────────────────
// 设置步骤状态
// ─────────────────────────────────────────────────────────────────
void ScanUploadDialog::setStep(int, const QString& label, bool ok)
{
    m_statusLabel->setText(label);
    m_statusLabel->setStyleSheet(QString(
        "font-size:12px;color:%1;font-weight:600;").arg(ok ? "#22C55E" : "#E53E3E"));
}

// ─────────────────────────────────────────────────────────────────
// 上传确认
// ─────────────────────────────────────────────────────────────────
void ScanUploadDialog::onUpload()
{
    if (m_selectedFile.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择扫描件文件。");
        return;
    }
    if (m_runtimePrivateKey.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入正确的 PIN 码验证身份。");
        return;
    }

    m_uploadBtn->setEnabled(false);
    m_logEdit->append("[上传] 开始处理扫描件...");

    // 步骤2：计算哈希
    setStep(2, "计算数字指纹...", true);
    QString hashBase64 = QString::fromUtf8(
        CryptoService::instance().sha256(
            QByteArray::fromBase64(m_hashLabel->text().toUtf8())
        ).toHex());
    // 重新从文件计算原始哈希
    QString rawHashHex = computeFileHash(m_selectedFile);
    QString rawHashB64 = QString::fromUtf8(
        QByteArray::fromHex(rawHashHex.toUtf8()).toBase64());
    m_logEdit->append("[验证] 再次确认文件哈希...");
    if (rawHashHex != m_hashLabel->text()) {
        setStep(2, "文件已被修改，拒绝签名！", false);
        QMessageBox::critical(this, "安全警告",
            "检测到文件已被修改，上传被拒绝！请重新选择原始扫描件。");
        m_uploadBtn->setEnabled(true);
        return;
    }
    setStep(2, "✓ 数字指纹确认", true);

    // 步骤3：加密存储
    setStep(3, "AES-256 加密存储...", true);
    QString encPath = encryptAndSaveFile(m_selectedFile, m_recordId);
    if (encPath.isEmpty()) {
        setStep(3, "加密存储失败", false);
        QMessageBox::critical(this, "错误", "扫描件加密存储失败。");
        m_uploadBtn->setEnabled(true);
        return;
    }
    setStep(3, "✓ 已加密存储", true);

    // 步骤4：数字签名
    setStep(4, "RSA-2048 数字签名中（请稍候）...", true);
    m_logEdit->append("[签名] 正在使用您的私钥签名...");
    QString sigB64 = signHash(rawHashB64);
    if (sigB64.isEmpty()) {
        setStep(4, "签名失败", false);
        QMessageBox::critical(this, "错误", "数字签名失败，请检查 PIN 码是否正确。");
        m_uploadBtn->setEnabled(true);
        return;
    }
    m_logEdit->append("[签名] ✓ 数字签名完成。签名将绑定您的身份，任何人无法伪造。");
    setStep(4, "✓ 签名完成", true);

    // 步骤5：保存记录
    setStep(5, "保存笔录记录...", true);
    QFileInfo fi(m_selectedFile);
    bool ok = saveScanRecord(m_recordId, m_recordDbId,
                             rawHashHex, sigB64, encPath, fi.size());
    if (!ok) {
        setStep(5, "数据库保存失败", false);
        QMessageBox::critical(this, "错误", "笔录记录保存失败。");
        m_uploadBtn->setEnabled(true);
        return;
    }
    setStep(5, "✓ 笔录已正式锁定", true);
    m_logEdit->append("[完成] ✓ 扫描件上传成功！笔录状态已锁定，不可再修改。");

    // 通过 P2P 网络广播扫描件索引，让其他节点记录备份
    if (P2PNodeService::instance().isRunning()) {
        P2PNodeService::instance().broadcastScanIndex(
            m_recordId,               // recordId
            sigB64,                   // 已签名的哈希
            fi.size(),                // 文件大小
            m_recordId                // 内容寻址 ID（本实现用 recordId）
        );
        m_logEdit->append("[P2P] ✓ 扫描件索引已广播至 P2P 网络，其他节点将自动记录备份。");
    } else {
        m_logEdit->append("[P2P] 当前节点未运行（未找到密钥），扫描件仅本地存储。");
    }

    QMessageBox::information(this, "上传成功",
        QString("扫描件已上传并完成数字签名！\n\n"
                "笔录编号：%1\n"
                "文件大小：%2 KB\n"
                "数字指纹：%3\n\n"
                "该笔录已正式锁定，任何篡改都能被检测。\n"
                "可在「档案管理」中查看并验证。").arg(m_recordId)
            .arg(fi.size() / 1024).arg(rawHashHex.left(32) + "..."));

    accept();
}
