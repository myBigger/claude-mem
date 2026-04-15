#include "approval_cmd.h"
#include "../formatter.h"
#include "../cli_utils.h"
#include <database/DatabaseManager.h>
#include <iostream>

static void printApprovalHelp() {
    std::cout << "用法: prison-cli approval <子命令> [选项]\n\n";
    std::cout << "子命令:\n";
    std::cout << "  list      列出待审批/已审批记录\n\n";
    std::cout << "选项:\n";
    std::cout << "  --record-id <id>   按档案记录ID筛选\n";
    std::cout << "  --status <status> 审批结果: pending approved rejected withdrawn\n";
    std::cout << "  --level <n>        审批层级: 1 或 2\n";
    std::cout << "  --format <fmt>     输出格式: table(默认) json yaml csv\n";
    std::cout << "  --limit <n>        限制结果数量，默认100\n";
}

static const QStringList APPROVAL_LIST_FIELDS = {
    "id", "record_id", "approval_level", "approver_id", "result",
    "opinion", "approval_time", "created_at"
};

static const QMap<QString, QString> APPROVAL_LIST_HEADERS = {
    {"id", "ID"}, {"record_id", "记录ID"}, {"approval_level", "审批层级"},
    {"approver_id", "审批人ID"}, {"result", "结果"},
    {"opinion", "意见"}, {"approval_time", "审批时间"}, {"created_at", "创建时间"}
};

int ApprovalCmd::run(const QStringList& args) {
    QString subCmd;
    QMap<QString, QString> opts;
    QStringList positionalArgs;

    for (int i = 0; i < args.size(); ++i) {
        const QString& arg = args[i];
        if (!arg.startsWith('-') && subCmd.isEmpty()) {
            subCmd = arg;
            positionalArgs = args.mid(i + 1);
            break;
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

    auto& db = DatabaseManager::instance();

    if (subCmd == "list" || subCmd.isEmpty()) {
        QString sql = "SELECT * FROM approvals WHERE 1=1";
        QMap<QString, QVariant> params;
        if (opts.contains("record-id")) {
            sql += " AND record_id = :recordId";
            params["recordId"] = opts["record-id"].toInt();
        }
        if (opts.contains("status")) {
            sql += " AND result = :result";
            params["result"] = opts["status"];
        }
        if (opts.contains("level")) {
            sql += " AND approval_level = :approvalLevel";
            params["approvalLevel"] = opts["level"].toInt();
        }
        sql += " ORDER BY id DESC LIMIT :limit";
        params["limit"] = limit;

        auto rows = db.select(sql, params);
        if (rows.isEmpty()) {
            std::cout << "（无数据）\n";
            return 0;
        }
        std::cout << Formatter::toTable(rows, APPROVAL_LIST_HEADERS).toUtf8().constData();
        return 0;

    } else if (subCmd == "-h" || subCmd == "--help") {
        printApprovalHelp();
        return 0;
    } else {
        std::cerr << "错误: 未知的子命令 '" << qPrintable(subCmd) << "'\n";
        printApprovalHelp();
        return 1;
    }
}
