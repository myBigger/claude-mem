# PrisonSIS Structured Output CLI Implementation

**Date**: 2026-04-15
**Project**: PrisonSIS (Prison Record Management System)
**Task**: Implement Priority 1 — Structured Output CLI mode

---

## Summary

为 PrisonSIS 实现了 `prison-cli` 命令行工具，支持 **table / json / yaml / csv** 四种结构化输出格式（参考 OpenCLI 设计）。

## What Was Built

### CLI 可执行文件
- **入口**: `src/cli/main.cpp` — 手动解析 argv，处理全局选项 `--db` 和 `--pin`
- **路由**: `criminals`, `records`, `approval`, `audit`, `export` 五个子命令
- **格式化**: `src/cli/formatter.cpp` — JSON / YAML / CSV / Table 四种输出

### 子命令
| 子命令 | 功能 |
|--------|------|
| `prison-cli criminals list` | 列出服刑人员，支持 `--district`、`--archived` |
| `prison-cli criminals search` | 搜索姓名，支持 `--criminal-name` |
| `prison-cli criminals show <id>` | 详情，支持 `--format json\|yaml\|csv` |
| `prison-cli records list` | 列出档案，支持 `--criminal-id`、`--type`、`--status` |
| `prison-cli records show <id>` | 详情（含解密内容）|
| `prison-cli approval list` | 审批记录，支持 `--record-id`、`--status`、`--level` |
| `prison-cli audit query` | 审计日志，支持 `--username`、`--action`、`--start-date`、`--end-date` |
| `prison-cli export criminals` | 导出服刑人员 CSV/JSON/YAML |
| `prison-cli export records` | 导出档案记录（含内容解密）|
| `prison-cli export all` | 导出全部数据 |

### 架构决策

1. **独立可执行文件**: `prison-cli` 与 `PrisonRecordTool` 共享源码（models, services, database），但只链接 Qt Core/Sql/Network
2. **手动参数解析**: 绕过 Qt `QCommandLineParser` 的 `--name` 与应用名冲突问题
3. **参数 map 不含 `:`**: `params["limit"]` 而非 `params[":limit"]`，`DatabaseManager::select()` 自动加 `:`
4. **PIN 解锁**: `KeyManager::instance().unlock(pin)` 后才可解密 `records.content`

## Key Files Created

- `src/cli/main.cpp`
- `src/cli/cli_utils.{h,cpp}`
- `src/cli/formatter.{h,cpp}`
- `src/cli/commands/criminals_cmd.{h,cpp}`
- `src/cli/commands/records_cmd.{h,cpp}`
- `src/cli/commands/approval_cmd.{h,cpp}`
- `src/cli/commands/audit_cmd.{h,cpp}`
- `src/cli/commands/export_cmd.{h,cpp}`

## Bug Fixes Applied

1. `KeyManager.h`: 移除 `#include <QDialog>`（CLI 无 GUI 依赖）
2. `CryptoService.cpp`: 移除 `#include <QMessageBox>`
3. `DatabaseManager.cpp`: 保持不变（参数 map 不带 `:` 前缀约定已确认）
4. CMakeLists: prison-cli 链接 `Qt6::Core Qt6::Sql Qt6::Network`，不含 GUI 模块

## Reference Libraries Evaluated

- **HKUDS/CLI-Anything** (30.8k ⭐): SKILL.md + 7-phase CLI pipeline → 参考了 SKILL 生成模式
- **jackwener/OpenCLI** (15.9k ⭐): 适配器架构 + `table/json/yaml/md/csv` 输出系统 → 直接实现参考

## Related Files

- `projects/PrisonSIS/CLAUDE.md` — 项目架构文档
- `src/ui/styles/dark_government.qss` — Government Blue 设计系统 QSS
- `DESIGN_SYSTEM.md` — 设计规范文档
