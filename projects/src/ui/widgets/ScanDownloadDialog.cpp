#include "ui/widgets/ScanDownloadDialog.h"
#include "services/P2PNodeService.h"
#include "services/CryptoService.h"
#include "services/AuthService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QGroupBox>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QApplication>
#include <QCryptographicHash>

// ─────────────────────────────────────────────────────────────────
// PIN → AES 密钥派生（与 ScanUploadDialog 保持一致）
// ─────────────────────────────────────────────────────────────────
static QByteArray deriveKeyFromPin(const QString& pin)
{
    QByteArray salt = "PrisonSIS_KeyDerivation_v1";
    QByteArray input = (pin + QString::fromUtf8(salt)).toUtf8();
    return CryptoService::instance().sha256(input);
}

// ─────────────────────────────────────────────────────────────────
// ScanDownloadDialog 实现
// ─────────────────────────────────────────────────────────────────
ScanDownloadDialog::ScanDownloadDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("从 P2P 网络下载扫描件");
    setMinimumSize(900, 600);
    resize(950, 650);
    setupUI();

    // 连接 P2P 信号，实时接收扫描索引广播
    auto& p2p = P2PNodeService::instance();
    connect(&p2p, &P2PNodeService::scanIndexReceived,
            this, &ScanDownloadDialog::onPeerScanIndexReceived);
    connect(&p2p, &P2PNodeService::runningChanged, this, [this](bool running) {
        m_networkStatusLabel->setText(running ? "🟢 P2P 在线" : "🔴 P2P 未启动");
        m_networkStatusLabel->setStyleSheet(running
            ? "color:#22C55E;font-weight:600;"
            : "color:#E53E3E;font-weight:600;");
        if (running) loadScanList();
    });

    // 加载当前已知的扫描列表
    loadScanList();
    if (p2p.isRunning()) {
        m_networkStatusLabel->setText("🟢 P2P 在线");
        m_networkStatusLabel->setStyleSheet("color:#22C55E;font-weight:600;");
    } else {
        m_networkStatusLabel->setText("🔴 P2P 未启动（无法下载）");
        m_networkStatusLabel->setStyleSheet("color:#E53E3E;font-weight:600;");
    }
}

ScanDownloadDialog::~ScanDownloadDialog()
{
    m_runtimePrivateKey.clear();
}

void ScanDownloadDialog::setupUI()
{
    QVBoxLayout* main = new QVBoxLayout(this);
    main->setSpacing(12);

    // ── 顶部说明 ──
    QLabel* header = new QLabel(
        QString::fromUtf8("⚠ 本功能用于从其他工作站的 P2P 节点下载扫描件。\n"
                           "扫描件以密文传输，只有持有私钥（知道 PIN）者才能解密查看。\n"
                           "请确保 P2P 网络正常（状态栏显示「在线」）后再下载。"));
    header->setWordWrap(true);
    header->setStyleSheet("color:#E6EDF3;background:#161B22;border:1px solid #21262D;border-radius:8px;padding:10px;font-size:13px;");
    main->addWidget(header);

    // ── 网络状态 + 刷新按钮 ──
    QHBoxLayout* statusRow = new QHBoxLayout;
    m_networkStatusLabel = new QLabel("🔴 正在连接...");
    m_networkStatusLabel->setStyleSheet("color:#8B949E;font-weight:600;font-size:13px;");
    QPushButton* refreshBtn = new QPushButton("刷新列表");
    refreshBtn->setStyleSheet("QPushButton{background:#1C2128;color:#E6EDF3;border:none;border-radius:4px;padding:6px 16px;font-size:13px;}"
        "QPushButton:hover{background:#30363D;}");
    statusRow->addWidget(new QLabel("P2P 网络状态："));
    statusRow->addWidget(m_networkStatusLabel);
    statusRow->addStretch();
    statusRow->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &ScanDownloadDialog::onRefresh);
    main->addLayout(statusRow);

    // ── 扫描列表 ──
    QGroupBox* tableBox = new QGroupBox("可下载的扫描件（来自 P2P 网络）");
    m_scanTable = new QTableWidget;
    m_scanTable->setColumnCount(5);
    m_scanTable->setHorizontalHeaderLabels({"笔录编号", "上传节点", "文件大小", "上传时间", "来源"});
    m_scanTable->horizontalHeader()->setStretchLastSection(true);
    m_scanTable->setSelectionBehavior(QTableWidget::SelectRows);
    m_scanTable->setSelectionMode(QTableWidget::SingleSelection);
    m_scanTable->setAlternatingRowColors(true);
    m_scanTable->setStyleSheet(
        "QTableWidget{background:#0A0E14;color:#E6EDF3;font-size:13px;gridline-color:#21262D;border:1px solid #21262D;border-radius:6px;}"
        "QTableWidget::item{color:#E6EDF3;padding:6px 8px;border-bottom:1px solid #21262D;}"
        "QTableWidget::item:selected{background:#0066CC1A;color:#FFFFFF;font-weight:600;}"
        "QHeaderView::section{background:#161B22;color:#8B949E;font-weight:600;padding:8px;border-bottom:1px solid #21262D;border-right:1px solid #21262D;font-size:12px;text-transform:uppercase;letter-spacing:0.5px;}"
        "QTableWidget::item:alternate{background:#0F1419;}");
    m_scanTable->setMinimumHeight(280);

    QVBoxLayout* tableLayout = new QVBoxLayout(tableBox);
    tableLayout->addWidget(m_scanTable);
    main->addWidget(tableBox, 1);

    // ── PIN 验证区 ──
    QGroupBox* pinBox = new QGroupBox("身份验证（解密需要 PIN）");
    QHBoxLayout* pinRow = new QHBoxLayout(pinBox);
    m_pinEdit = new QLineEdit;
    m_pinEdit->setPlaceholderText("输入您的 PIN 码（6位数字）...");
    m_pinEdit->setEchoMode(QLineEdit::Password);
    m_pinEdit->setMaximumWidth(200);
    m_pinStatusLabel = new QLabel("未验证");
    m_pinStatusLabel->setStyleSheet("color:#F59E0B;font-size:12px;");

    m_downloadBtn = new QPushButton("从网络下载选中扫描件");
    m_downloadBtn->setStyleSheet(
        "QPushButton{background:#0066CC;"
        "color:#FFFFFF;font-weight:600;padding:8px 20px;border:none;border-radius:4px;font-size:13px;}"
        "QPushButton:disabled{background:#1C2128;color:#6E7681;}"
        "QPushButton:hover{background:#0052A3;}");
    m_downloadBtn->setEnabled(false);

    m_exportBtn = new QPushButton("导出 PDF");
    m_exportBtn->setStyleSheet(
        "QPushButton{background:#22C55E;color:#FFFFFF;font-weight:600;"
        "padding:8px 20px;border:none;border-radius:4px;font-size:13px;}"
        "QPushButton:disabled{background:#1C2128;color:#6E7681;}"
        "QPushButton:hover{background:#16A34A;}");
    m_exportBtn->setEnabled(false);

    pinRow->addWidget(new QLabel("PIN 码："));
    pinRow->addWidget(m_pinEdit, 1);
    pinRow->addWidget(m_pinStatusLabel);
    pinRow->addWidget(m_downloadBtn);
    pinRow->addWidget(m_exportBtn);
    main->addWidget(pinBox);

    // ── 操作日志 ──
    QLabel* logLabel = new QLabel("操作日志：");
    m_logEdit = new QTextEdit;
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(90);
    m_logEdit->setStyleSheet("font-family:'JetBrains Mono','SF Mono','Consolas',monospace;font-size:11px;background:#0A0E14;color:#8B949E;border:1px solid #21262D;border-radius:6px;padding:4px;");
    main->addWidget(logLabel);
    main->addWidget(m_logEdit);

    // ── 信号连接 ──
    connect(m_scanTable, &QTableWidget::itemSelectionChanged, this, [this]() {
        m_selectedRecordId.clear();
        auto items = m_scanTable->selectedItems();
        if (!items.isEmpty()) {
            int row = items.first()->row();
            QTableWidgetItem* recItem = m_scanTable->item(row, 0);
            if (recItem) m_selectedRecordId = recItem->text();
        }
        m_downloadBtn->setEnabled(!m_selectedRecordId.isEmpty()
            && P2PNodeService::instance().isRunning());
    });

    connect(m_pinEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.length() >= 6 && decryptPrivateKey(text)) {
            m_pinStatusLabel->setText("✓ PIN 验证成功");
            m_pinStatusLabel->setStyleSheet("color:#22C55E;font-size:12px;font-weight:600;");
        } else if (!text.isEmpty()) {
            m_pinStatusLabel->setText("✗ PIN 错误或未生成密钥");
            m_pinStatusLabel->setStyleSheet("color:#E53E3E;font-size:12px;font-weight:600;");
        } else {
            m_pinStatusLabel->setText("未验证");
            m_pinStatusLabel->setStyleSheet("color:#F59E0B;font-size:12px;");
        }
    });

    connect(m_downloadBtn, &QPushButton::clicked, this, &ScanDownloadDialog::onDownload);
    connect(m_exportBtn, &QPushButton::clicked, this, &ScanDownloadDialog::onExportPdf);
}

// ─────────────────────────────────────────────────────────────────
// 加载当前已知扫描列表（从 P2PNodeService）
// ─────────────────────────────────────────────────────────────────
void ScanDownloadDialog::loadScanList()
{
    m_scanPeersMap.clear();
    auto& p2p = P2PNodeService::instance();

    if (!p2p.isRunning()) {
        m_logEdit->append("[系统] P2P 未启动，无法加载扫描列表。");
        return;
    }

    // 获取全网已知扫描
    m_scanPeersMap = p2p.allKnownScanPeers();

    refreshScanList();
    m_logEdit->append(QString("[系统] 已加载 %1 个扫描件索引").arg(m_scanPeersMap.size()));
}

void ScanDownloadDialog::refreshScanList()
{
    m_scanTable->setRowCount(0);

    auto& p2p = P2PNodeService::instance();
    const QString myNodeId = p2p.isRunning()
        ? QString::fromUtf8(
              CryptoService::instance().sha256(
                  AuthService::instance().currentUser().userId.toUtf8()
              ).toHex().left(16))
        : "";

    int row = 0;
    for (auto it = m_scanPeersMap.constBegin(); it != m_scanPeersMap.constEnd(); ++it) {
        const QString& recordId = it.key();
        const QStringList& peers = it.value();

        for (const QString& peerNodeId : peers) {
            // 获取索引详情（从 peerScanIndices）
            qint64 fileSize = 0;
            qint64 timestamp = 0;

            auto indices = p2p.allKnownScanPeers();
            Q_UNUSED(indices);

            bool isMine = (peerNodeId == myNodeId);
            QString source = isMine ? "本节点" : QString("节点 %1").arg(peerNodeId);

            m_scanTable->insertRow(row);
            m_scanTable->setItem(row, 0, new QTableWidgetItem(recordId));
            m_scanTable->setItem(row, 1, new QTableWidgetItem(
                isMine ? "本节点" : peerNodeId));
            m_scanTable->setItem(row, 2, new QTableWidgetItem(
                fileSize > 0 ? QString("%1 KB").arg(fileSize / 1024) : "未知"));
            m_scanTable->setItem(row, 3, new QTableWidgetItem(
                timestamp > 0
                    ? QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd hh:mm")
                    : "未知"));
            m_scanTable->setItem(row, 4, new QTableWidgetItem(source));
            ++row;
        }
    }

    if (row == 0) {
        m_logEdit->append("[提示] 网络中暂无其他扫描件索引，请确保其他节点已上传扫描件。");
    }
}

// ─────────────────────────────────────────────────────────────────
// 收到新的扫描索引广播
// ─────────────────────────────────────────────────────────────────
void ScanDownloadDialog::onPeerScanIndexReceived(const QString& recordId,
                                                   const QString& authorNodeId,
                                                   qint64 fileSize,
                                                   const QString& fileCID)
{
    Q_UNUSED(fileCID);
    if (!m_scanPeersMap.contains(recordId))
        m_scanPeersMap[recordId] = QStringList();
    if (!m_scanPeersMap[recordId].contains(authorNodeId))
        m_scanPeersMap[recordId].append(authorNodeId);

    m_logEdit->append(QString("[P2P] 发现新扫描件：笔录 %1（来自节点 %2，%3 KB）")
        .arg(recordId).arg(authorNodeId).arg(fileSize / 1024));

    refreshScanList();
}

// ─────────────────────────────────────────────────────────────────
// 刷新按钮
// ─────────────────────────────────────────────────────────────────
void ScanDownloadDialog::onRefresh()
{
    m_logEdit->append("[刷新] 正在刷新 P2P 扫描列表...");
    loadScanList();
    m_logEdit->append(QString("[刷新] 完成，共 %1 个扫描件").arg(m_scanPeersMap.size()));
}

// ─────────────────────────────────────────────────────────────────
// 下载选中扫描件（通过 P2P 拉取）
// ─────────────────────────────────────────────────────────────────
void ScanDownloadDialog::onDownload()
{
    if (m_selectedRecordId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先在列表中选择要下载的扫描件。");
        return;
    }
    if (m_runtimePrivateKey.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入正确的 PIN 码验证身份。");
        return;
    }
    if (!P2PNodeService::instance().isRunning()) {
        QMessageBox::warning(this, "提示", "P2P 未启动，无法下载。");
        return;
    }

    auto& p2p = P2PNodeService::instance();
    QStringList peers = p2p.peersHavingScan(m_selectedRecordId);

    if (peers.isEmpty()) {
        QMessageBox::information(this, "提示",
            QString("未找到笔录 %1 的扫描件。\n"
                    "可能原因：\n"
                    "  1. 持有扫描件的节点当前不在线\n"
                    "  2. 该笔录尚未上传扫描件\n"
                    "请稍后重试。").arg(m_selectedRecordId));
        return;
    }

    m_downloadBtn->setEnabled(false);
    m_logEdit->append(QString("[下载] 正在从节点 %1 拉取笔录 %2 ...").arg(peers.first()).arg(m_selectedRecordId));

    // 连接 scanPulled 信号，一次性获取结果
    auto slot = [this, recordId = m_selectedRecordId](const QString& rid,
                                                       const QByteArray& encData, bool ok) {
        if (rid != recordId) return;
        auto& p2p = P2PNodeService::instance();
        disconnect(&p2p, &P2PNodeService::scanPulled, nullptr, nullptr);

        if (!ok || encData.isEmpty()) {
            m_logEdit->append("[下载] ❌ 拉取失败，对端节点无可用连接。");
            QMessageBox::critical(this, "下载失败", "从 P2P 网络拉取扫描件失败。\n请确认对方节点在线。");
            m_downloadBtn->setEnabled(true);
            return;
        }

        m_logEdit->append(QString("[下载] 收到加密数据 %1 字节，开始解密...").arg(encData.size()));

        // 解密扫描件
        QByteArray plainData;
        if (!decryptScanData(encData, plainData) || plainData.isEmpty()) {
            m_logEdit->append("[解密] ❌ 解密失败，PIN 可能不正确。");
            QMessageBox::critical(this, "解密失败",
                "扫描件密文已下载，但 PIN 解密失败。\n"
                "请确认您的 PIN 码是否正确。");
            m_downloadBtn->setEnabled(true);
            return;
        }

        m_downloadedData = plainData;
        m_logEdit->append(QString("[解密] ✓ 解密成功，原始文件大小：%1 KB").arg(plainData.size() / 1024));

        // 保存到本地（临时目录）
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/p2p/downloads";
        QDir().mkpath(tempDir);
        QString outPath = tempDir + "/" + m_selectedRecordId + "_decrypted.pdf";
        QFile f(outPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(plainData);
            f.close();
            m_logEdit->append(QString("[保存] 已保存至：%1").arg(outPath));
        }

        QMessageBox::information(this, "下载成功",
            QString("扫描件已解密下载！\n\n笔录编号：%1\n"
                    "文件大小：%2 KB\n\n"
                    "点击「导出 PDF」将文件保存到指定位置。").arg(m_selectedRecordId)
                .arg(plainData.size() / 1024));

        m_exportBtn->setEnabled(true);
        m_downloadBtn->setEnabled(true);
    };

    connect(&p2p, &P2PNodeService::scanPulled, this, slot);

    // 尝试从第一个在线节点拉取
    p2p.pullScanFromPeer(peers.first(), m_selectedRecordId);
}

// ─────────────────────────────────────────────────────────────────
// 解密扫描件（加密格式：{ encKey(rsa), iv, cipher(base64), originalSize })
// ─────────────────────────────────────────────────────────────────
bool ScanDownloadDialog::decryptScanData(const QByteArray& encryptedData,
                                         QByteArray& outPlain)
{
    QJsonParseError err;
    QJsonObject obj = QJsonDocument::fromJson(encryptedData, &err).object();
    if (err.error != QJsonParseError::NoError || obj.isEmpty()) {
        // 可能 encryptedData 本身就是 base64 编码的 JSON
        QByteArray decoded = QByteArray::fromBase64(encryptedData);
        obj = QJsonDocument::fromJson(decoded, &err).object();
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "[ScanDownload] JSON 解析失败";
            return false;
        }
    }

    QByteArray encKeyB64  = obj["encKey"].toString().toUtf8();
    QByteArray encKeyData = QByteArray::fromBase64(encKeyB64);

    // 用 RSA 私钥解密 AES 密钥
    QByteArray aesKey = CryptoService::instance().rsaDecrypt(
        encKeyB64, m_runtimePrivateKey);
    if (aesKey.isEmpty()) {
        qWarning() << "[ScanDownload] RSA 解密 AES 密钥失败";
        return false;
    }

    // 解密密文（AES-256-CBC，密文格式：iv + cipher → base64）
    QByteArray cipherB64 = obj["cipher"].toString().toUtf8();
    QByteArray cipherData = QByteArray::fromBase64(cipherB64);
    if (cipherData.isEmpty()) {
        qWarning() << "[ScanDownload] 密文 base64 解码失败";
        return false;
    }

    // 从密文头部提取 IV（加密时 iv 预置在密文前面）
    if (cipherData.size() < 16) {
        qWarning() << "[ScanDownload] 密文太短";
        return false;
    }
    QByteArray iv = cipherData.left(16);
    QByteArray cipher = cipherData.mid(16);

    QByteArray plain = CryptoService::instance().decryptAES(cipher, aesKey);
    if (plain.isEmpty()) {
        qWarning() << "[ScanDownload] AES 解密失败";
        return false;
    }

    outPlain = plain;
    return true;
}

// ─────────────────────────────────────────────────────────────────
// 导出 PDF
// ─────────────────────────────────────────────────────────────────
void ScanDownloadDialog::onExportPdf()
{
    if (m_downloadedData.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先下载并解密扫描件。");
        return;
    }

    QString defaultName = m_selectedRecordId + "_decrypted.pdf";
    QString path = QFileDialog::getSaveFileName(this, "导出 PDF 文件",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultName,
        "PDF 文件 (*.pdf)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "错误", "无法写入文件：" + path);
        return;
    }
    f.write(m_downloadedData);
    f.close();

    m_logEdit->append(QString("[导出] ✓ 已导出至：%1").arg(path));
    QMessageBox::information(this, "导出成功", "PDF 已保存至：\n" + path);
}

// ─────────────────────────────────────────────────────────────────
// 用 PIN 解密私钥
// ─────────────────────────────────────────────────────────────────
bool ScanDownloadDialog::decryptPrivateKey(const QString& pin)
{
    auto& auth = AuthService::instance();
    QString uid = auth.currentUser().userId;
    QString keysDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/keys/" + uid;
    QString privPath = keysDir + "/private_key.enc";

    QFile f(privPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    m_encryptedPrivateKey = f.readAll();
    f.close();

    QByteArray aesKey = deriveKeyFromPin(pin);
    QByteArray iv;
    iv.resize(16);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(pin.toUtf8());
    iv = hash.result().left(16);

    // 解密（AES 输出格式：iv + cipher → base64，decryptAES 两参数版自动提取 IV）
    QByteArray dec = CryptoService::instance().decryptAES(m_encryptedPrivateKey, aesKey);
    if (dec.isEmpty()) return false;

    m_runtimePrivateKey = QString::fromUtf8(dec);
    return !m_runtimePrivateKey.isEmpty();
}
