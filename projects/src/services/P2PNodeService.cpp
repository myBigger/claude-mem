#include "services/P2PNodeService.h"
#include "services/CryptoService.h"
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QTimer>
#include <QApplication>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDataStream>
#include <QDebug>

// ──────────────────────────────────────────────
// 常量
// ──────────────────────────────────────────────
static const quint16 UDP_DISCOVERY_PORT  = 25555;
static const quint16 TCP_TRANSPORT_PORT  = 25556;
static const int DISCOVERY_INTERVAL_MS   = 5000;   // 每5秒广播一次 HELLO
static const int NODES_GOSSIP_INTERVAL_MS = 15000; // 每15秒广播一次 NODES（含索引）
static const int PEER_TIMEOUT_MS         = 30000;  // 30秒无响应视为离线
static const QString P2P_MAGIC           = "PRSIS_P2P_v1";

// ──────────────────────────────────────────────
// 工具：PEM → 节点 ID
// ──────────────────────────────────────────────
static QString computeNodeId(const QString& publicKeyPEM)
{
    QByteArray hash = CryptoService::instance().sha256(publicKeyPEM.toUtf8());
    return QString::fromUtf8(hash.toHex().left(16));
}

// ──────────────────────────────────────────────
// 工具：本地存储路径
// ──────────────────────────────────────────────
static QString localDataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/p2p";
}

// ──────────────────────────────────────────────
// PeerTcpConnection 方法实现（类已在头文件声明）
// ──────────────────────────────────────────────
PeerTcpConnection::PeerTcpConnection(QTcpSocket* sock, const QString& peerNodeId, QObject* parent)
    : QObject(parent), m_sock(sock), m_peerNodeId(peerNodeId)
{
    connect(m_sock, &QTcpSocket::readyRead, this, &PeerTcpConnection::onReadyRead);
    connect(m_sock, &QTcpSocket::disconnected, this, &PeerTcpConnection::onDisconnected);
}

QString PeerTcpConnection::peerNodeId() const { return m_peerNodeId; }

void PeerTcpConnection::onReadyRead()
{
    m_recvBuffer.append(m_sock->readAll());
    processBuffer();
}

void PeerTcpConnection::onDisconnected() { deleteLater(); }

void PeerTcpConnection::processBuffer()
{
    while (m_recvBuffer.size() >= 4) {
        QDataStream ds(m_recvBuffer);
        ds.setByteOrder(QDataStream::BigEndian);
        quint32 len = 0;
        ds >> len;
        if (len > 10 * 1024 * 1024) { m_sock->close(); return; }
        if (m_recvBuffer.size() < 4 + static_cast<int>(len)) break;
        QByteArray jsonData = m_recvBuffer.mid(4, len);
        m_recvBuffer = m_recvBuffer.mid(4 + len);

        QJsonParseError err;
        QJsonObject req = QJsonDocument::fromJson(jsonData, &err).object();
        if (err.error != QJsonParseError::NoError) continue;
        handleRequest(req);
    }
}

void PeerTcpConnection::handleRequest(const QJsonObject& req)
{
    QString type = req["type"].toString();
    qDebug() << "[P2P-TCP] 请求:" << type << "来自节点:" << m_peerNodeId;
    if (type == "PULL_SCAN") {
        handlePullScan(req);
    } else if (type == "GET_INDICES") {
        emit indicesRequested(m_peerNodeId);
    }
}

void PeerTcpConnection::handlePullScan(const QJsonObject& req)
{
    QString recordId = req["recordId"].toString();
    qDebug() << "[P2P-TCP] 拉取请求:" << recordId << "来自:" << m_peerNodeId;

    QString scansDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                      + "/p2p/scans";
    QString foundPath;
    QDir d(scansDir);
    for (const QString& sub : d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString p = scansDir + "/" + sub + "/" + recordId + "_scan.enc";
        if (QFile::exists(p)) { foundPath = p; break; }
    }

    QJsonObject resp;
    resp["magic"] = P2P_MAGIC;
    if (!foundPath.isEmpty()) {
        QFile f(foundPath);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray enc = f.readAll();
            f.close();
            resp["type"] = "SCAN_DATA";
            resp["encryptedData"] = QString::fromUtf8(enc.toBase64());
            qDebug() << "[P2P-TCP] 发送扫描件:" << enc.size() << "字节";
        }
    } else {
        resp["type"] = "ERROR";
        resp["error"] = "Scan file not found on this node";
    }
    sendJson(resp);
    m_sock->close();
}

void PeerTcpConnection::sendJson(const QJsonObject& obj)
{
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    quint32 len = data.size();
    QByteArray lenBytes;
    QDataStream ds(&lenBytes, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << len;
    m_sock->write(lenBytes + data);
    m_sock->flush();
}

// ──────────────────────────────────────────────
// P2PNodeService 实现
// ──────────────────────────────────────────────
P2PNodeService::P2PNodeService(QObject* parent)
    : QObject(parent)
{}

P2PNodeService::~P2PNodeService()
{
    stop();
}

P2PNodeService& P2PNodeService::instance()
{
    static P2PNodeService inst;
    return inst;
}

// ──────────────────────────────────────────────
// 启动
// ──────────────────────────────────────────────
void P2PNodeService::start(const QString& userId, const QString& publicKeyPEM)
{
    if (m_running) return;
    m_userId      = userId;
    m_publicKeyPEM = publicKeyPEM;
    m_nodeId       = computeNodeId(publicKeyPEM);

    qDebug() << "[P2P] 启动，节点ID:" << m_nodeId;
    QDir().mkpath(localDataDir());
    QDir().mkpath(localDataDir() + "/scans");
    QDir().mkpath(localDataDir() + "/indices");

    // 从本地加载已有的扫描索引
    QVector<ScanIndexEntry> stored = loadLocalScanIndices();
    for (const ScanIndexEntry& e : stored) {
        // 恢复时无法区分是本节点还是对等节点上传的，统一归入本节点索引
        m_myScanIndex[e.recordId] = e;
    }

    // UDP 绑定（节点发现 + 索引广播）
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
    bool bound = m_udpSocket->bind(QHostAddress::AnyIPv4, UDP_DISCOVERY_PORT,
                                    QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!bound) bound = m_udpSocket->bind(QUdpSocket::ShareAddress);
    qDebug() << "[P2P] UDP 绑定:" << bound << "端口:" << m_udpSocket->localPort();

    connect(m_udpSocket, &QUdpSocket::readyRead, this, [this]() {
        while (m_udpSocket->hasPendingDatagrams()) {
            QNetworkDatagram d = m_udpSocket->receiveDatagram();
            processIncomingDatagram(d.data(), d.senderAddress(), d.senderPort());
        }
    });

    // TCP 服务器（供其他节点拉取扫描件）
    m_tcpServer = new QTcpServer(this);
    if (!m_tcpServer->listen(QHostAddress::AnyIPv4, TCP_TRANSPORT_PORT))
        m_tcpServer->listen(QHostAddress::AnyIPv4, 0);
    m_tcpPort = m_tcpServer->serverPort();
    qDebug() << "[P2P] TCP 端口:" << m_tcpPort;

    connect(m_tcpServer, &QTcpServer::newConnection, this, [this]() {
        while (m_tcpServer->hasPendingConnections()) {
            QTcpSocket* sock = m_tcpServer->nextPendingConnection();
            PeerTcpConnection* conn = new PeerTcpConnection(sock, "unknown", this);
            m_connections.insert(conn);

            // 从已知的 peer 中根据 IP 找到 nodeId
            QString foundNodeId = "unknown";
            for (auto it = m_peers.constBegin(); it != m_peers.constEnd(); ++it) {
                if (it.value().addr == sock->peerAddress()) {
                    foundNodeId = it.key();
                    break;
                }
            }
            conn = new PeerTcpConnection(sock, foundNodeId, this);
            connect(conn, &PeerTcpConnection::indicesRequested,
                    this, &P2PNodeService::onPeerRequestedIndices);
            connect(conn, &QObject::destroyed, this, [this](QObject* o) {
                m_connections.remove(static_cast<PeerTcpConnection*>(o));
            });
        }
    });

    // 定时广播
    m_discoveryTimer = new QTimer(this);
    connect(m_discoveryTimer, &QTimer::timeout, this, &P2PNodeService::broadcastDiscovery);
    m_discoveryTimer->start(DISCOVERY_INTERVAL_MS);

    m_peerTimeoutTimer = new QTimer(this);
    connect(m_peerTimeoutTimer, &QTimer::timeout, this, &P2PNodeService::onPeerTimeoutCheck);
    m_peerTimeoutTimer->start(PEER_TIMEOUT_MS / 2);

    m_gossipTimer = new QTimer(this);
    connect(m_gossipTimer, &QTimer::timeout, this, &P2PNodeService::broadcastNODES);
    m_gossipTimer->start(NODES_GOSSIP_INTERVAL_MS);

    m_running = true;
    emit runningChanged(true);

    // 启动后立即广播一次
    QTimer::singleShot(100, this, &P2PNodeService::broadcastDiscovery);
    QTimer::singleShot(200, this, &P2PNodeService::broadcastNODES);
}

// ──────────────────────────────────────────────
// 停止
// ──────────────────────────────────────────────
void P2PNodeService::stop()
{
    if (!m_running) return;

    m_discoveryTimer->stop();
    m_peerTimeoutTimer->stop();
    m_gossipTimer->stop();
    m_udpSocket->close();
    m_tcpServer->close();

    qDeleteAll(m_connections);
    m_connections.clear();
    m_peers.clear();

    m_running = false;
    emit runningChanged(false);
    qDebug() << "[P2P] 节点服务已停止";
}

// ──────────────────────────────────────────────
// 广播 HELLO（UDP）
// ──────────────────────────────────────────────
void P2PNodeService::broadcastDiscovery()
{
    if (!m_udpSocket) return;

    QJsonObject msg;
    msg["magic"]   = P2P_MAGIC;
    msg["type"]    = "HELLO";
    msg["nodeId"]  = m_nodeId;
    msg["userId"]  = m_userId;
    msg["tcpPort"] = m_tcpPort;

    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    QByteArray packet = P2P_MAGIC.toUtf8() + "\n" + data;

    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            iface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
                QHostAddress bc = entry.broadcast();
                if (!bc.isNull())
                    m_udpSocket->writeDatagram(packet, bc, UDP_DISCOVERY_PORT);
            }
        }
    }
    m_udpSocket->writeDatagram(packet, QHostAddress::Broadcast, UDP_DISCOVERY_PORT);
}

// ──────────────────────────────────────────────
// 广播本节点持有的扫描件索引（NODES 消息，携带 SCAN_INDEX 列表）
// ──────────────────────────────────────────────
void P2PNodeService::broadcastNODES()
{
    if (!m_udpSocket) return;

    // 收集本节点拥有的扫描件 recordId 列表（包含已加密的）
    QJsonArray indices;
    for (auto it = m_myScanIndex.constBegin(); it != m_myScanIndex.constEnd(); ++it) {
        const ScanIndexEntry& e = it.value();
        QJsonObject idx;
        idx["recordId"]    = e.recordId;
        idx["signedHash"]  = e.authorNodeId; // 自引用，签名暂用 authorNodeId 代替
        idx["fileSize"]    = e.fileSize;
        idx["fileCID"]     = e.fileCID;
        idx["timestamp"]   = e.timestamp;
        indices.append(idx);
    }

    QJsonObject msg;
    msg["magic"]   = P2P_MAGIC;
    msg["type"]     = "NODES";
    msg["nodeId"]   = m_nodeId;
    msg["userId"]   = m_userId;
    msg["tcpPort"]  = m_tcpPort;
    msg["indices"]  = indices;  // 本节点拥有的扫描件索引列表

    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    QByteArray packet = P2P_MAGIC.toUtf8() + "\n" + data;
    m_udpSocket->writeDatagram(packet, QHostAddress::Broadcast, UDP_DISCOVERY_PORT);
    qDebug() << "[P2P] 广播 NODES，本节点持有" << indices.size() << "个扫描件索引";
}

// ──────────────────────────────────────────────
// 广播本节点刚上传的扫描件索引（单条 SCAN_INDEX）
// ──────────────────────────────────────────────
void P2PNodeService::broadcastScanIndex(const QString& recordId,
                                        const QString& signedHashBase64,
                                        qint64 fileSize,
                                        const QString& fileCID)
{
    if (!m_udpSocket) return;

    ScanIndexEntry entry;
    entry.recordId     = recordId;
    entry.authorNodeId  = m_nodeId;  // 本节点上传的，作者就是本节点
    entry.fileSize     = fileSize;
    entry.fileCID      = fileCID;
    entry.timestamp     = QDateTime::currentSecsSinceEpoch();

    // 存入本节点索引
    m_myScanIndex[recordId] = entry;
    saveScanIndexToLocal(entry, m_nodeId);

    // 广播 SCAN_INDEX
    QJsonObject msg;
    msg["magic"]      = P2P_MAGIC;
    msg["type"]       = "SCAN_INDEX";
    msg["nodeId"]     = m_nodeId;
    msg["recordId"]   = recordId;
    msg["signedHash"] = signedHashBase64;
    msg["fileSize"]   = fileSize;
    msg["fileCID"]    = fileCID;
    msg["timestamp"]  = entry.timestamp;

    QByteArray data = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    QByteArray packet = P2P_MAGIC.toUtf8() + "\n" + data;
    m_udpSocket->writeDatagram(packet, QHostAddress::Broadcast, UDP_DISCOVERY_PORT);
    qDebug() << "[P2P] 广播 SCAN_INDEX:" << recordId << "本节点:" << m_nodeId;

    // 同时通过 NODES 再次广播（让对等节点更新索引）
    QTimer::singleShot(50, this, &P2PNodeService::broadcastNODES);
}

// ──────────────────────────────────────────────
// 处理收到的 UDP 数据报
// ──────────────────────────────────────────────
void P2PNodeService::processIncomingDatagram(const QByteArray& data,
                                             const QHostAddress& from,
                                             quint16)
{
    if (data.isEmpty()) return;

    QList<QByteArray> parts = data.split('\n');
    if (parts.isEmpty() || parts[0] != P2P_MAGIC.toUtf8()) return;
    if (parts.size() < 2) return;

    QJsonParseError err;
    QJsonObject obj = QJsonDocument::fromJson(parts[1], &err).object();
    if (err.error != QJsonParseError::NoError) return;

    QString type = obj["type"].toString();
    QString fromNodeId = obj["nodeId"].toString();
    if (fromNodeId == m_nodeId) return;  // 忽略自己发的

    qDebug() << "[P2P] 收到 UDP:" << type << "从节点:" << fromNodeId << "@" << from.toString();

    if (type == "HELLO") {
        PeerNode peer;
        peer.nodeId   = fromNodeId;
        peer.addr     = from;
        peer.tcpPort  = obj["tcpPort"].toInt(TCP_TRANSPORT_PORT);
        peer.lastSeen = QDateTime::currentMSecsSinceEpoch();
        bool isNew = !m_peers.contains(fromNodeId);
        m_peers[fromNodeId] = peer;
        if (isNew) {
            emit peerOnline(fromNodeId, from);
            qDebug() << "[P2P] 节点加入:" << fromNodeId;
        }

        // HELLO 来了后，请求该节点的扫描件索引（通过 TCP GET_INDICES）
        QTimer::singleShot(300, this, [this, peer]() {
            requestPeerIndices(peer);
        });

    } else if (type == "SCAN_INDEX") {
        // 收到单条扫描索引 → 记录到对等索引，并通知 UI
        QString recordId = obj["recordId"].toString();
        ScanIndexEntry entry;
        entry.recordId    = recordId;
        entry.authorNodeId = fromNodeId;   // 上传者节点
        entry.fileSize    = obj["fileSize"].toInteger();
        entry.fileCID     = obj["fileCID"].toString();
        entry.timestamp   = obj["timestamp"].toInteger();

        // 存入对等索引（每个节点都可能上传同一个 recordId，取最新时间戳）
        if (!m_peerScanIndices[fromNodeId].contains(recordId) ||
            m_peerScanIndices[fromNodeId][recordId].timestamp < entry.timestamp) {
            m_peerScanIndices[fromNodeId][recordId] = entry;
        }
        saveScanIndexToLocal(entry, fromNodeId);

        qDebug() << "[P2P] 记录扫描索引:" << recordId << "上传者:" << fromNodeId;
        emit scanIndexReceived(recordId, fromNodeId, entry.fileSize, entry.fileCID);

    } else if (type == "NODES") {
        // NODES 消息：节点信息 + 该节点持有的全部扫描件索引
        PeerNode peer;
        peer.nodeId   = fromNodeId;
        peer.addr     = from;
        peer.tcpPort  = obj["tcpPort"].toInt(TCP_TRANSPORT_PORT);
        peer.lastSeen = QDateTime::currentMSecsSinceEpoch();
        bool isNew = !m_peers.contains(fromNodeId);
        m_peers[fromNodeId] = peer;
        if (isNew) emit peerOnline(fromNodeId, from);

        // 更新该节点的扫描件索引列表
        QJsonArray indices = obj["indices"].toArray();
        for (const QJsonValue& v : indices) {
            QJsonObject idx = v.toObject();
            QString recordId = idx["recordId"].toString();
            ScanIndexEntry entry;
            entry.recordId    = recordId;
            entry.authorNodeId = fromNodeId;
            entry.fileSize    = idx["fileSize"].toInteger();
            entry.fileCID     = idx["fileCID"].toString();
            entry.timestamp   = idx["timestamp"].toInteger();

            if (!m_peerScanIndices[fromNodeId].contains(recordId) ||
                m_peerScanIndices[fromNodeId][recordId].timestamp < entry.timestamp) {
                m_peerScanIndices[fromNodeId][recordId] = entry;
                saveScanIndexToLocal(entry, fromNodeId);
                emit scanIndexReceived(recordId, fromNodeId, entry.fileSize, entry.fileCID);
            }
        }
        qDebug() << "[P2P] NODES 更新，节点" << fromNodeId << "持有" << indices.size() << "个扫描件";
    }
}

// ──────────────────────────────────────────────
// 通过 TCP 向指定节点请求其扫描件索引
// ──────────────────────────────────────────────
void P2PNodeService::requestPeerIndices(const PeerNode& peer)
{
    QTcpSocket* sock = new QTcpSocket(this);
    connect(sock, &QTcpSocket::connected, this, [=]() {
        QJsonObject req;
        req["magic"] = P2P_MAGIC;
        req["type"]  = "GET_INDICES";
        req["nodeId"] = m_nodeId;
        QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);
        quint32 len = data.size();
        QByteArray lenBytes;
        QDataStream ds(&lenBytes, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << len;
        sock->write(lenBytes + data);
        sock->flush();
        sock->disconnectFromHost();
    });
    connect(sock, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError) {
        qWarning() << "[P2P] 请求节点索引失败:" << peer.nodeId << sock->errorString();
        sock->deleteLater();
    });
    sock->connectToHost(peer.addr, peer.tcpPort);
}

// ──────────────────────────────────────────────
// 对等节点通过 TCP 请求本节点索引 → 发送 NODES 广播回去
// ──────────────────────────────────────────────
void P2PNodeService::onPeerRequestedIndices(const QString& peerNodeId)
{
    Q_UNUSED(peerNodeId);
    qDebug() << "[P2P] 收到索引请求，回复 NODES";
    QTimer::singleShot(50, this, &P2PNodeService::broadcastNODES);
}

// ──────────────────────────────────────────────
// 节点超时检查
// ──────────────────────────────────────────────
void P2PNodeService::onPeerTimeoutCheck()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QStringList toRemove;
    for (auto it = m_peers.constBegin(); it != m_peers.constEnd(); ++it) {
        if (now - it.value().lastSeen > PEER_TIMEOUT_MS)
            toRemove.append(it.key());
    }
    for (const QString& nid : toRemove) {
        m_peers.remove(nid);
        m_peerScanIndices.remove(nid);
        qDebug() << "[P2P] 节点超时移除:" << nid;
        emit peerOffline(nid);
    }
}

// ──────────────────────────────────────────────
// 查询：哪些节点有此扫描件
// ──────────────────────────────────────────────
QStringList P2PNodeService::peersHavingScan(const QString& recordId) const
{
    QStringList result;
    // 本节点自己
    if (m_myScanIndex.contains(recordId))
        result.append(m_nodeId);
    // 对等节点
    for (auto it = m_peerScanIndices.constBegin(); it != m_peerScanIndices.constEnd(); ++it) {
        if (it.value().contains(recordId))
            result.append(it.key());
    }
    return result;
}

// ──────────────────────────────────────────────
// 拉取扫描件（从指定节点，TCP）
// ──────────────────────────────────────────────
void P2PNodeService::pullScanFromPeer(const QString& peerNodeId, const QString& recordId)
{
    PeerNode peer;
    bool found = false;
    for (auto it = m_peers.constBegin(); it != m_peers.constEnd(); ++it) {
        if (it.key() == peerNodeId) { peer = it.value(); found = true; break; }
    }
    if (!found) {
        qWarning() << "[P2P] 未找到节点:" << peerNodeId;
        emit scanPulled(recordId, QByteArray(), false);
        return;
    }

    qDebug() << "[P2P] 从节点" << peerNodeId << "@" << peer.addr.toString()
             << "拉取扫描件:" << recordId;

    QTcpSocket* sock = new QTcpSocket(this);
    connect(sock, &QTcpSocket::connected, this, [=]() {
        QJsonObject req;
        req["magic"]    = P2P_MAGIC;
        req["type"]     = "PULL_SCAN";
        req["recordId"] = recordId;
        req["nodeId"]   = m_nodeId;
        QByteArray reqData = QJsonDocument(req).toJson(QJsonDocument::Compact);
        quint32 len = reqData.size();
        QByteArray lenBytes;
        QDataStream ds(&lenBytes, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << len;
        sock->write(lenBytes + reqData);
        sock->flush();
    });

    connect(sock, &QTcpSocket::readyRead, this, [=]()
    {
        QByteArray all = sock->readAll();
        if (all.size() > 4) {
            QByteArray jsonData = all.mid(4);
            QJsonObject resp = QJsonDocument::fromJson(jsonData).object();
            if (resp["type"].toString() == "SCAN_DATA") {
                QByteArray encData = QByteArray::fromBase64(
                    resp["encryptedData"].toString().toUtf8());
                qDebug() << "[P2P] 拉取成功，收到" << encData.size() << "字节";
                emit scanPulled(recordId, encData, true);
            } else {
                qWarning() << "[P2P] 拉取失败:" << resp["error"].toString();
                emit scanPulled(recordId, QByteArray(), false);
            }
        }
        sock->deleteLater();
    });

    connect(sock, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError) {
        qWarning() << "[P2P] TCP 错误:" << sock->errorString();
        emit scanPulled(recordId, QByteArray(), false);
        sock->deleteLater();
    });

    sock->connectToHost(peer.addr, peer.tcpPort);
}

// ──────────────────────────────────────────────
// 全网扫描索引（供 UI 显示）
// ──────────────────────────────────────────────
QMap<QString, QStringList> P2PNodeService::allKnownScanPeers() const
{
    QMap<QString, QStringList> result;
    // 本节点
    for (auto it = m_myScanIndex.constBegin(); it != m_myScanIndex.constEnd(); ++it)
        result[it.key()].append(m_nodeId);
    // 对等节点
    for (auto pit = m_peerScanIndices.constBegin(); pit != m_peerScanIndices.constEnd(); ++pit) {
        for (auto it = pit.value().constBegin(); it != pit.value().constEnd(); ++it)
            result[it.key()].append(pit.key());
    }
    return result;
}

// ──────────────────────────────────────────────
// 本节点上传的扫描件列表
// ──────────────────────────────────────────────
QStringList P2PNodeService::myScanRecordIds() const
{
    QStringList result;
    for (auto it = m_myScanIndex.constBegin(); it != m_myScanIndex.constEnd(); ++it)
        result.append(it.key());
    return result;
}

// ──────────────────────────────────────────────
// 保存扫描索引到本地
// ──────────────────────────────────────────────
void P2PNodeService::saveScanIndexToLocal(const ScanIndexEntry& entry, const QString& ownerNodeId)
{
    QString path = localDataDir() + "/indices/" + ownerNodeId + "_" + entry.recordId + ".json";
    QJsonObject obj;
    obj["recordId"]     = entry.recordId;
    obj["authorNodeId"] = entry.authorNodeId;
    obj["ownerNodeId"]  = ownerNodeId;
    obj["fileSize"]     = entry.fileSize;
    obj["fileCID"]      = entry.fileCID;
    obj["timestamp"]    = entry.timestamp;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        f.close();
    }
}

QVector<P2PNodeService::ScanIndexEntry> P2PNodeService::loadLocalScanIndices() const
{
    QVector<ScanIndexEntry> result;
    QDir dir(localDataDir() + "/indices");
    for (const QString& fname : dir.entryList({"*.json"})) {
        QFile f(dir.filePath(fname));
        if (f.open(QIODevice::ReadOnly)) {
            QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
            ScanIndexEntry e;
            e.recordId    = obj["recordId"].toString();
            e.authorNodeId = obj["authorNodeId"].toString();
            e.fileSize   = obj["fileSize"].toInteger();
            e.fileCID    = obj["fileCID"].toString();
            e.timestamp  = obj["timestamp"].toInteger();
            result.append(e);
        }
    }
    return result;
}

