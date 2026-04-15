# 监狱笔录系统 (Prison Interrogation & Record System)
# SPEC v1.0 — PrisonSIS

## 1. 项目概述

**项目名称：** 监狱笔录系统（PrisonSIS）
**项目类型：** 桌面应用 / Qt Widgets + C++ / SQLite 本地数据库
**核心功能：** 为监狱司法系统提供完整的罪犯笔录制作、案件管理、嫌疑人信息管理、文档生成与审计日志功能
**目标用户：** 监狱信息科人员、审讯记录员、法制科人员、系统管理员

---

## 2. 技术栈

| 层级 | 技术选型 |
|------|---------|
| 编程语言 | C++17 |
| GUI框架 | Qt 6.x (QWidgets) |
| 数据库 | SQLite 3 (Qt SQL Module) |
| 构建系统 | CMake 3.20+ |
| 文档导出 | QPrinter + QPainter (A4 PDF/打印) |
| 加密 | OpenSSL (AES-256-CBC + RSA-2048 + SHA-256) |
| P2P 网络 | QUdpSocket (端口 25555) + QTcpServer (端口 25556) |
| 编译器 | GCC / Clang |

---

## 3. 功能模块

### 3.1 用户认证与权限管理
- 登录窗口：用户名 + 密码（本地 PBKDF2 哈希）
- 角色：系统管理员、法制科、审讯员、信息科（只读）
- 基于角色的访问控制（RBAC）
- 所有操作记入审计日志

### 3.2 案件管理 (Case Management)
- 新建案件（案号、案件名称、案件类型、立案日期、承办人、摘要）
- 案件列表（支持按日期、类型、状态筛选；支持关键字搜索）
- 案件状态：侦查中 → 审查起诉 → 已结案
- 案件详情查看与编辑

### 3.3 嫌疑人/罪犯信息管理 (Suspect Management)
- 基本信息：姓名、身份证号、出生日期、性别、民族、籍贯
- 羁押信息：羁押日期、羁押地点、案件编号关联
- 附加信息：照片（本地文件路径）、指纹编号、身体特征
- 嫌疑人列表（支持与案件联动筛选）

### 3.4 笔录制作与管理 (Interrogation Records)
- 新建笔录（关联案件编号 + 嫌疑人）
- 笔录模板（讯问笔录、询问笔录、辨认笔录）
- 笔录内容编辑：时间地点、自动时间戳、问答格式录入
- 问答分段录入（问/答交替，含说话人标注）
- 笔录状态：草稿 → 待审核 → 已审核 → 已归档
- 笔录预览（HTML 渲染）

### 3.5 文档生成与导出 (Document Export)
- 笔录导出为规范化文本文件
- 案件报告生成（PDF via HTML 打印）
- 打印预览

### 3.6 审计日志 (Audit Log)
- 记录所有关键操作（登录/登出、新建/修改/删除、导出）
- 日志字段：时间戳、操作人、操作类型、IP地址（本地）、操作详情
- 日志查询（按日期范围、操作人、操作类型筛选）
- 日志不可删除/不可篡改

### 3.7 去中心化扫描件存储 (P2P Scan Sync)
**解决的问题：** 管教员在任意工作站都能访问自己的已签名扫描件
**方案：** 轻量级 P2P 同步（无中央服务器，扫描件全程加密）

#### 3.7.1 节点发现（UDP 广播，端口 25555）
- 节点启动时每 5 秒广播 HELLO 消息（UDP 广播）
- 消息格式：`{magic, type:"HELLO", nodeId, userId, tcpPort}`
- 节点 ID = SHA-256(用户公钥 PEM) 前 16 位（无需中央 CA）
- 30 秒无响应视为节点离线

#### 3.7.2 扫描件传输（TCP，端口 25556）
- TCP 服务器接受其他节点的拉取请求
- 协议帧：4 字节 Big-Endian 长度头 + JSON body
- 请求类型：PULL_SCAN（根据 recordId 查找本地扫描件）
- 响应类型：SCAN_DATA / ERROR

#### 3.7.3 扫描件索引广播（SCAN_INDEX）
- 上传扫描件后，通过 UDP 广播 SCAN_INDEX
- 格式：`{recordId, signedHash, fileSize, fileCID, timestamp}`
- 所有在线节点本地记录索引

#### 3.7.4 加密存储设计
- 扫描件使用 AES-256-CBC 加密（AES 密钥随机生成）
- AES 密钥用上传者公钥 RSA-2048 加密，嵌入密文文件
- 密文文件结构：`{encKey, iv, cipher, originalSize}` → JSON → Base64
- **只有持有对应私钥的人才能解密**，P2P 网络分发的是密文
- 私钥由用户 PIN（SHA-256 派生 AES 密钥）加密本地存储

#### 3.7.6 下载 UI（ScanDownloadDialog）
- 实时显示 P2P 网络中各节点持有的扫描件索引
- 支持按节点/笔录编号筛选可下载扫描件
- 从指定节点拉取加密扫描件（PULL_SCAN 协议）
- 用用户私钥（PIN 保护）解密扫描件，导出为 PDF 文件
- 通过 P2PNodeService::scanPulled 信号异步接收下载结果

#### 3.7.5 存储路径
```
~/.local/share/PrisonSystem/
├── keys/{userId}/
│   ├── public_key.pem          # RSA 公钥
│   └── private_key.enc         # PIN 加密的 RSA 私钥
└── p2p/
    ├── scans/{nodeId}/         # 加密扫描件
    │   └── {recordId}_scan.enc
    └── indices/                 # 扫描件索引（JSON）
        └── {recordId}.json
```

---

### 3.8 应用层字段加密 (Field-Level Encryption)

**解决的问题：** 数据库文件被直接复制走时，笔录内容明文暴露

**方案：** 应用层 AES-256-CBC 加密，PIN 保护主密钥

#### 3.8.1 主密钥管理 (KeyManager)
- 首次启动：用户设置 6 位 PIN，程序生成随机 AES-256 主密钥
- 主密钥由 PIN 派生 AES 密钥加密存储（`config/master.key.enc`）
- 每次启动：输入 PIN → 解锁主密钥到内存 → 程序关闭自动清零
- 修改 PIN：需验证旧 PIN，重新加密主密钥

#### 3.8.2 笔录正文加密
- `records.content` 明文 → `KeyManager.encryptField()` → AES-256-Encrypt → 存数据库
- 所有读取 `records.content` 的 widget 均通过 `KeyManager.decryptContent()` 自动解密
- 旧记录（`content_encrypted=0`）降级显示为明文，兼容历史数据
- `content_encrypted=1` 标记新记录为已加密

#### 3.8.3 配置文件格式
```
~/.local/share/PrisonSystem/config/master.key.enc
{
  "version": 1,
  "salt": "<随机盐，Base64>",
  "verification": "<加密的固定字符串，PIN正确性校验>",
  "encryptedKey": "<加密后的 MasterKey，Base64>",
  "iv": "<加密IV，Base64>"
}
```

#### 3.8.4 攻击效果
| 攻击方式 | 结果 |
|---------|------|
| 直接复制 `prison.db` | content 列是密文乱码，无法读取 ✅ |
| 改数据库字段 | 解密后内容损坏，立刻暴露被篡改 |
| 不知道 PIN | 解不开主密钥，无法解密任何笔录 ✅ |
| 程序关闭后内存消失 | 主密钥从内存消失 ✅ |

---

### 4.1 整体风格
- **主色调：** 深蓝色 #1F4E79（政务蓝）、浅灰 #F0F0F0（背景）
- **字体：** "Microsoft YaHei UI" / "PingFang SC" / "Noto Sans CJK SC"
- **布局：** 主窗口 + 左侧导航栏 + 右侧内容区（QMdiArea 多文档）
- **风格：** 传统桌面应用，清晰层次，数据密集型

### 4.2 窗口结构
```
主窗口 (MainWindow)
├── 顶部工具栏 (QToolBar) — 快捷操作
├── 左侧导航面板 (QTreeWidget / QListWidget)
│   ├── 案件管理
│   ├── 嫌疑人管理
│   ├── 笔录制作
│   ├── 文档导出
│   └── 系统设置
├── 中央工作区 (QMdiArea)
│   └── 多文档子窗口
└── 底部状态栏 (QStatusBar)
```

### 4.3 主要子窗口
| 子窗口 | 用途 |
|--------|------|
| LoginDialog | 登录界面 |
| CaseListWidget | 案件列表 + 搜索 |
| CaseDetailWidget | 案件详情 / 编辑 |
| SuspectListWidget | 嫌疑人列表 |
| SuspectDetailWidget | 嫌疑人详情 / 编辑 |
| InterrogationEditorWidget | 笔录编辑器（核心） |
| ScanUploadDialog | 扫描件上传对话框（加密 + 签名） |
| ScanDownloadDialog | 扫描件下载对话框（P2P 拉取 + 解密） |
| AuditLogWidget | 审计日志查看器 |
| UserManageWidget | 用户管理（管理员） |
| DocumentExportWidget | 文档导出 |

---

## 5. 数据模型

### 5.1 数据库表结构

```sql
-- 用户表
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL CHECK(role IN ('admin','legal','interrogator','readonly')),
    full_name TEXT,
    department TEXT,
    created_at TEXT DEFAULT (datetime('now')),
    last_login TEXT
);

-- 案件表
CREATE TABLE cases (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    case_number TEXT UNIQUE NOT NULL,
    case_name TEXT NOT NULL,
    case_type TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'investigation',
    filing_date TEXT NOT NULL,
    handler TEXT,
    summary TEXT,
    created_by INTEGER REFERENCES users(id),
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now'))
);

-- 嫌疑人表
CREATE TABLE suspects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    id_number TEXT UNIQUE,
    birth_date TEXT,
    gender TEXT CHECK(gender IN ('male','female','other')),
    ethnicity TEXT,
    native_place TEXT,
    custody_date TEXT,
    custody_location TEXT,
    case_id INTEGER REFERENCES cases(id),
    photo_path TEXT,
    fingerprint_id TEXT,
    physical_marks TEXT,
    created_by INTEGER REFERENCES users(id),
    created_at TEXT DEFAULT (datetime('now'))
);

-- 笔录表
CREATE TABLE records (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    record_id TEXT UNIQUE NOT NULL,
    record_type TEXT NOT NULL,
    criminal_id INTEGER NOT NULL REFERENCES criminals(id),
    criminal_name TEXT,
    record_date TEXT, record_location TEXT,
    interrogator_id TEXT, recorder_id TEXT, present_persons TEXT,
    content TEXT,
    content_encrypted INTEGER NOT NULL DEFAULT 0,  -- 1=已加密，0=明文（历史兼容）
    status TEXT NOT NULL DEFAULT 'Draft',
    version INTEGER NOT NULL DEFAULT 1,
    created_by INTEGER REFERENCES users(id),
    created_at TEXT DEFAULT (datetime('now')),
    updated_at TEXT DEFAULT (datetime('now')),
    scan_file_path TEXT,
    scan_hash TEXT,
    scan_signature TEXT,
    scan_file_size INTEGER,
    scan_uploaded_at TEXT,
    scan_uploaded_by INTEGER REFERENCES users(id),
    -- 审批字段
    approver1_id INTEGER, approver2_id INTEGER,
    approver1_opinion TEXT, approver2_opinion TEXT,
    approver1_result TEXT, approver2_result TEXT,
    reject_reason TEXT,
    -- 签名字段（现场手写签名）
    signed_interrogator INTEGER NOT NULL DEFAULT 0,
    signed_recorder INTEGER NOT NULL DEFAULT 0,
    signed_subject INTEGER NOT NULL DEFAULT 0
);

-- 审计日志表
CREATE TABLE audit_logs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER REFERENCES users(id),
    username TEXT,
    action TEXT NOT NULL,
    module TEXT NOT NULL,
    target_id INTEGER,
    details TEXT,
    ip_address TEXT DEFAULT '127.0.0.1',
    timestamp TEXT DEFAULT (datetime('now'))
);
```

---

## 6. 项目文件结构

```
PrisonSIS/
├── CMakeLists.txt
├── SPEC.md
├── README.md
├── docs/
│   ├── 产品介绍.md / 安全性说明.md / 用户手册.md / 快速入门.md
├── include/
│   ├── models/
│   │   ├── User.h / Criminal.h / Record.h / Approval.h / Template.h / AuditLog.h
│   ├── database/
│   │   └── DatabaseManager.h
│   ├── services/
│   │   ├── AuthService.h / AuditService.h
│   │   ├── CryptoService.h         # AES-256 + RSA-2048 + SHA-256
│   │   ├── PrisonApiService.h
│   │   └── P2PNodeService.h         # 去中心化节点服务
│   ├── ui/
│   │   ├── MainWindow.h / LoginDialog.h
│   │   └── widgets/
│   │       ├── CriminalListWidget.h / CriminalDetailWidget.h
│   │       ├── CriminalEditDialog.h / CriminalFetchDialog.h
│   │       ├── RecordEditorWidget.h  # 笔录编辑 + 打印 + 扫描上传
│   │       ├── LinedTextEdit.h       # 信签纸背景
│   │       ├── RecordThemeSelector.h
│   │       ├── StatisticsWidget.h
│   │       ├── ApprovalWidget.h / ArchiveWidget.h
│   │       ├── TemplateWidget.h / UserManageWidget.h
│   │       ├── BackupWidget.h / LogViewerWidget.h
│   │       ├── ScanUploadDialog.h    # 扫描件加密上传对话框
│   │       └── ScanDownloadDialog.h  # 扫描件 P2P 下载对话框
│   └── utils/
│       ├── PasswordHasher.h / DateUtils.h / EncryptUtil.h
└── src/
    ├── main.cpp                     # 应用入口，启动 P2P 节点
    ├── models/
    ├── database/
    ├── services/
    │   ├── CryptoService.cpp / P2PNodeService.cpp
    │   └── PrisonApiService.cpp
    ├── ui/
    │   └── widgets/
    └── utils/
```

---

## 7. 验收标准

1. **编译通过**：cmake + make 零错误零警告
2. **登录流程**：输入正确凭据可进入主界面，错误3次锁定
3. **CRUD 完整**：案件、嫌疑人、笔录的新建/查看/编辑/删除功能完整
4. **数据持久化**：所有数据保存到 SQLite，重启后数据存在
5. **权限隔离**：只读用户无法新建/编辑/删除
6. **审计日志**：每次关键操作均有日志记录，可查询
7. **文档导出**：笔录可导出为规范文本文件
8. **中文支持**：所有界面文字为中文，UTF-8 编码正确

---

*文档版本：1.0 | 最后更新：2026-04-09*
