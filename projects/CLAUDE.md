# PrisonSIS — 监狱信息管理系统

## 项目概述

Qt6 Widgets (C++17) 桌面应用，监狱服刑人员信息管理与档案笔录系统。

## 技术栈

- **GUI**: Qt6 Widgets (QWidgets)
- **数据库**: SQLite via Qt SQL Module (`QSqlDatabase`)
- **加密**: AES-256 字段加密（`KeyManager` 单例，PIN 保护）
- **CLI**: 独立 `prison-cli` 可执行文件（仅链接 Qt Core/Sql/Network）
- **构建**: CMake + make
- **平台**: Linux (Ubuntu 22.04)

## 架构要点

### 数据库层
- `DatabaseManager` 单例，`select/selectOne/insert/update/remove` API
- **参数约定**: map key 不含 `:` 前缀（如 `params["limit"]`），`select()` 自动加 `:`
- 表名: `users`, `criminals`, `records`, `approvals`, `templates`, `audit_logs`
- 列名: snake_case（如 `criminal_id`, `record_type`, `sentence_years`）

### 加密层
- `KeyManager` 单例，首次使用需 `setup(pin)` 初始化
- CLI 使用需 `KeyManager::instance().unlock(pin)` 解锁
- `KeyManager::decryptContent(fieldValue)` 透明解密

### CLI (`prison-cli`)
```
prison-cli criminals list --db <path>
prison-cli criminals search --criminal-name <name>
prison-cli criminals show <id> --format json|yaml|csv
prison-cli records list --criminal-id <id> --status pending
prison-cli records show <id> --pin <code>
prison-cli approval list --record-id <id>
prison-cli audit query --username <user> --limit 50
prison-cli export criminals --format csv > out.csv
```

### 设计系统（Government Blue）
- 主色: `#0066CC`，背景: `#0A0E14` / `#0F1419` / `#161B22`
- 语义色: 成功 `#22C55E`，危险 `#E53E3E`，警告 `#F59E0B`
- 参考: `VoltAgent/awesome-design-md`

## 重要约束

1. **列名用 snake_case** — `criminal_id`, `record_type`, `created_at`
2. **DB 参数 map key 不含 `:`** — `params["limit"]`，SQL 用 `:limit`
3. **CLI 需 PIN 解锁** — `KeyManager::instance().unlock(pin)` 后才可解密
4. **共享源码** — CLI 和 GUI 共享 `models/`、`database/`、`services/`
5. **Qt 应用名** — 必须在 `QCoreApplication` 构造前调用 `setApplicationName`，否则 `--name` 被 Qt 自动消费

## 数据库路径

- 开发环境: `~/.local/share/PrisonSystem/prison.db`
- CLI 指定: `--db /path/to/prison.db`
