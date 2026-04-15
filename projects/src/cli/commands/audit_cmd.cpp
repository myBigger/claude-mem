#include "audit_cmd.h"
#include "../formatter.h"
#include "../cli_utils.h"
#include <database/DatabaseManager.h>
#include <services/AuditService.h>
#include <iostream>

static void printAuditHelp() {
    std::cout << "用法: prison-cli audit <子命令> [选项]\n\n";
    std::cout << "子命令:\n";
    std::cout << "  query     查询审计日志\n\n";
    std::cout << "选项:\n";
    std::cout << "  --username <name>    按用户名筛选\n";
    std::cout << "  --action <action>   按操作类型筛选\n";
    std::cout << "  --module <module>   按模块筛选\n";
    std::cout << "  --start-date <date> 开始日期 (YYYY-MM-DD)\n";
    std::cout << "  --end-date <date>   结束日期 (YYYY-MM-DD)\n";
    std::cout << "  --format <fmt>      输出格式: table(默认) json yaml csv\n";
    std::cout << "  --limit <n>         限制结果数量，默认100\n";
    std::cout << "\n操作类型示例: 登录 登出 新增 编辑 删除 查看 审批通过 审批驳回 数据导出\n";
}

static const QMap<QString, QString> AUDIT_LIST_HEADERS = {
    {"id", "ID"}, {"username", "用户名"}, {"action", "操作"},
    {"module", "模块"}, {"target_type", "对象类型"}, {"target_id", "对象ID"},
    {"details", "详情"}, {"ip_address", "IP地址"}, {"timestamp", "时间"}
};

int AuditCmd::run(const QStringList& args) {
    QString subCmd;
    QMap<QString, QString> opts;

    for (int i = 0; i < args.size(); ++i) {
        const QString& arg = args[i];
        if (!arg.startsWith('-') && subCmd.isEmpty()) {
            subCmd = arg;
        } else if (arg.startsWith("--")) {
            if (arg.contains('=')) {
                QStringList parts = arg.mid(2).split('=');
                opts[parts[0]] = parts[1];
            } else {
                opts[arg.mid(2)] = (i + 1 < args.size() && !args[i + 1].startsWith('-'))
                                   ? args[++i] : QString();
            }
        } else if (arg.startsWith('-')) {
            opts[arg.mid(1)] = (i + 1 < args.size() && !args[i + 1].startsWith('-'))
                                ? args[++i] : QString();
        }
    }

    OutputFormat fmt = Formatter::parseFormat(opts.value("format"));
    int limit = opts.value("limit").toInt();
    if (limit <= 0) limit = 100;

    if (subCmd == "query" || subCmd == "list" || subCmd.isEmpty()) {
        auto rows = AuditService::instance().queryLogs(
            opts.value("start-date"),
            opts.value("end-date"),
            opts.value("username"),
            opts.value("action"),
            limit,
            0
        );
        if (rows.isEmpty()) {
            std::cout << "（无数据）\n";
            return 0;
        }
        std::cout << Formatter::toTable(rows, AUDIT_LIST_HEADERS).toUtf8().constData();
        return 0;

    } else if (subCmd == "-h" || subCmd == "--help") {
        printAuditHelp();
        return 0;
    } else {
        std::cerr << "错误: 未知的子命令 '" << qPrintable(subCmd) << "'\n";
        printAuditHelp();
        return 1;
    }
}
