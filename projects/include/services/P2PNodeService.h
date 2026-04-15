#ifndef P2PNODESERVICE_H
#define P2PNODESERVICE_H

#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QTimer>
#include <QNetworkDatagram>
#include <QJsonObject>

// ──────────────────────────────────────────────
// PeerTcpConnection：TCP 连接处理（声明在头文件以支持 moc）
// ──────────────────────────────────────────────
class PeerTcpConnection : public QObject
{
    Q_OBJECT
public:
    PeerTcpConnection(class QTcpSocket* sock, const QString& peerNodeId, QObject* parent = nullptr);
    QString peerNodeId() const;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processBuffer();
    void handleRequest(const QJsonObject& req);
    void handlePullScan(const QJsonObject& req);
    void sendJson(const QJsonObject& obj);

    class QTcpSocket* m_sock;
    QString m_peerNodeId;
    QByteArray m_recvBuffer;

signals:
    void indicesRequested(const QString& peerNodeId);
};

// ──────────────────────────────────────────────
// P2PNodeService — PrisonSIS 去中心化节点服务
//
// 功能：
// - UDP 广播发现同局域网内其他节点（端口 25555）
// - TCP 传输加密扫描件数据（端口 25556）
// - 广播/订阅扫描件索引（SCAN_INDEX + NODES 消息）
// - 按需拉取加密扫描件密文
//
// 设计原则：
// - 扫描件数据全程加密，只有持有私钥者能解密
// - 节点身份由 RSA 公钥哈希标识，无需中央 CA
// - 所有通信在监狱内网中，无外部暴露风险
// ──────────────────────────────────────────────
class P2PNodeService : public QObject
{
    Q_OBJECT

public:
    static P2PNodeService& instance();

    // ── 生命周期 ──
    void start(const QString& userId, const QString& publicKeyPEM);
    void stop();
    bool isRunning() const { return m_running; }

    // ── 广播扫描件索引（上传扫描件时调用）──
    // 广播后，所有在线节点都会记录：此公钥在此时间点上传了该笔录
    void broadcastScanIndex(const QString& recordId,
                           const QString& signedHashBase64,
                           qint64 fileSize,
                           const QString& fileCID);

    // ── 查询：哪些节点有此扫描件的索引（可从多节点拉取）──
    // 返回节点 ID 列表
    QStringList peersHavingScan(const QString& recordId) const;

    // ── 拉取指定扫描件密文（从指定节点，TCP）──
    void pullScanFromPeer(const QString& peerNodeId,
                          const QString& recordId);

    // ── 获取本节点已知的全网扫描索引（供 UI 显示）──
    QMap<QString, QStringList> allKnownScanPeers() const;

    // ── 获取本节点自己上传的扫描件列表 ──
    QStringList myScanRecordIds() const;

signals:
    // 新节点加入网络
    void peerOnline(const QString& peerNodeId, const QHostAddress& addr);
    // 节点离线
    void peerOffline(const QString& peerNodeId);
    // 收到扫描件索引广播
    void scanIndexReceived(const QString& recordId,
                           const QString& authorNodeId,
                           qint64 fileSize,
                           const QString& fileCID);
    // 扫描件密文拉取完成
    void scanPulled(const QString& recordId, const QByteArray& encryptedData, bool success);
    // 服务状态变化
    void runningChanged(bool running);

private:
    explicit P2PNodeService(QObject* parent = nullptr);
    ~P2PNodeService();
    Q_DISABLE_COPY(P2PNodeService)

    // 内部数据结构
    struct PeerNode {
        QString nodeId;          // SHA-256(公钥) 前16位
        QHostAddress addr;
        quint16 tcpPort;
        qint64 lastSeen;
    };

    struct ScanIndexEntry {
        QString recordId;
        QString authorNodeId;
        qint64 fileSize;
        QString fileCID;        // 内容寻址ID（简化用 recordId）
        qint64 timestamp;
    };

    void broadcastDiscovery();
    void processIncomingDatagram(const QByteArray& data, const QHostAddress& from, quint16 port);
    void onPeerTimeoutCheck();
    void saveScanIndexToLocal(const ScanIndexEntry& entry, const QString& ownerNodeId);
    QVector<ScanIndexEntry> loadLocalScanIndices() const;
    void broadcastMyIndices();
    void broadcastNODES();

    bool m_running = false;
    QString m_userId;
    QString m_nodeId;               // 本节点 ID = SHA-256(公钥PEM)
    QString m_publicKeyPEM;
    quint16 m_tcpPort = 25556;

    QMap<QString, PeerNode> m_peers; // nodeId → PeerNode

    // 本节点自己上传的扫描件索引（recordId → entry）
    QMap<QString, ScanIndexEntry> m_myScanIndex;

    // 各对等节点上报的扫描件索引（peerNodeId → {recordId → entry}）
    QMap<QString, QMap<QString, ScanIndexEntry>> m_peerScanIndices;

    class QUdpSocket* m_udpSocket = nullptr;
    class QTcpServer* m_tcpServer = nullptr;
    QTimer* m_discoveryTimer = nullptr;
    QTimer* m_peerTimeoutTimer = nullptr;
    QTimer* m_gossipTimer = nullptr;

    // 已连接 TCP 客户端
    QSet<class PeerTcpConnection*> m_connections;

    void requestPeerIndices(const PeerNode& peer);
    void onPeerRequestedIndices(const QString& peerNodeId);

    friend class P2PDiscoveryAgent;
    friend class P2PTransportAgent;
};

#endif // P2PNODESERVICE_H
