#include "records_cmd.h"
#include "../formatter.h"
#include "../cli_utils.h"
#include <database/DatabaseManager.h>
#include <iostream>

static void printRecordsHelp() {
    std::cout << "用法: prison-cli records <子命令> [选项]\n\n";
    std::cout << "子命令:\n";
    std::cout << "  list      列出档案记录\n";
    std::cout << "  search    搜索记录\n";
    std::cout << "  show      显示详情（需 --pin 解密加密内容）\n\n";
    std::cout << "选项:\n";
    std::cout << "  --criminal-id <id>   按服刑人员ID筛选\n";
    std::cout << "  --type <type>       记录类型: entry_note daily_talk case_investigation\n";
    std::cout << "                      admin_penalty sentence_evaluate exit_education\n";
    std::cout << "                      medical_check family_visit custom\n";
    std::cout << "  --status <status>   状态: draft pending in_approval approved rejected\n";
    std::cout << "  --format <fmt>      输出格式: table(默认) json yaml csv\n";
    std::cout << "  --limit <n>         限制结果数量，默认100\n";
}

static const QStringList RECORD_LIST_FIELDS = {
    "id", "record_id", "record_type", "criminal_id", "criminal_name",
    "record_date", "interrogator_id", "status", "created_at"
};

static const QMap<QString, QString> RECORD_LIST_HEADERS = {
    {"id", "ID"}, {"record_id", "编号"}, {"record_type", "类型"},
    {"criminal_id", "人员ID"}, {"criminal_name", "人员姓名"},
    {"record_date", "日期"}, {"interrogator_id", "记录人"},
    {"status", "状态"}, {"created_at", "创建时间"}
};

static const QStringList RECORD_DETAIL_FIELDS = {
    "id", "record_id", "record_type", "criminal_id", "criminal_name",
    "record_date", "record_location", "interrogator_id", "recorder_id",
    "present_persons", "content", "signed_interrogator", "signed_recorder",
    "signed_subject", "status", "approver1_id", "approver1_opinion",
    "approver1_result", "approver2_id", "approver2_opinion", "approver2_result",
    "reject_reason", "version", "created_by", "created_at", "updated_at"
};

static const QMap<QString, QString> RECORD_DETAIL_HEADERS = {
    {"id", "ID"}, {"record_id", "编号"}, {"record_type", "类型"},
    {"criminal_id", "人员ID"}, {"criminal_name", "人员姓名"},
    {"record_date", "日期"}, {"record_location", "地点"},
    {"interrogator_id", "讯问人"}, {"recorder_id", "记录人"},
    {"present_persons", "在场人员"}, {"content", "内容"},
    {"signed_interrogator", "讯问人已签名"}, {"signed_recorder", "记录人已签名"},
    {"signed_subject", "被讯问人已签名"}, {"status", "状态"},
    {"approver1_id", "审批人1ID"}, {"approver1_opinion", "审批人1意见"},
    {"approver1_result", "审批人1结果"}, {"approver2_id", "审批人2ID"},
    {"approver2_opinion", "审批人2意见"}, {"approver2_result", "审批人2结果"},
    {"reject_reason", "驳回原因"}, {"version", "版本"},
    {"created_by", "创建人"}, {"created_at", "创建时间"}, {"updated_at", "更新时间"}
};

static QString recordTypeFilter(const QString& t) {
    if (t == "entry_note")          return "EntryNote";
    if (t == "daily_talk")           return "DailyTalk";
    if (t == "case_investigation")  return "CaseInvestigation";
    if (t == "admin_penalty")        return "AdminPenalty";
    if (t == "sentence_evaluate")    return "SentenceEvaluate";
    if (t == "exit_education")      return "ExitEducation";
    if (t == "medical_check")       return "MedicalCheck";
    if (t == "family_visit")         return "FamilyVisit";
    if (t == "custom")              return "Custom";
    return t;
}

static QString recordStatusFilter(const QString& s) {
    if (s == "draft")       return "Draft";
    if (s == "pending")     return "PendingApproval";
    if (s == "in_approval") return "InApproval";
    if (s == "approved")    return "Approved";
    if (s == "rejected")    return "Rejected";
    return s;
}

// Decrypt sensitive fields in a row copy
static QMap<QString, QVariant> decryptRecordRow(const QMap<QString, QVariant>& row) {
    QMap<QString, QVariant> out = row;
    if (out.contains("content")) {
        out["content"] = CliUtils::decryptField(row.value("content").toString());
    }
    return out;
}

int RecordsCmd::run(const QStringList& args) {
    // Manual argument parsing
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

    // Also parse options in positionalArgs
    QStringList remainingPositional;
    for (int i = 0; i < positionalArgs.size(); ++i) {
        const QString& arg = positionalArgs[i];
        if (arg.startsWith("--")) {
            if (arg.contains('=')) {
                QStringList parts = arg.mid(2).split('=');
                opts[parts[0]] = parts[1];
            } else {
                opts[arg.mid(2)] = (i + 1 < positionalArgs.size() && !positionalArgs[i + 1].startsWith('-'))
                                   ? positionalArgs[++i] : QString();
            }
        } else if (arg.startsWith('-')) {
            opts[arg.mid(1)] = (i + 1 < positionalArgs.size() && !positionalArgs[i + 1].startsWith('-'))
                                ? positionalArgs[++i] : QString();
        } else {
            remainingPositional.append(arg);
        }
    }
    positionalArgs = remainingPositional;

    OutputFormat fmt = Formatter::parseFormat(opts.value("format"));
    int limit = opts.value("limit").toInt();
    if (limit <= 0) limit = 100;

    auto& db = DatabaseManager::instance();

    if (subCmd == "list" || subCmd == "search") {
        QString sql = "SELECT * FROM records WHERE 1=1";
        QMap<QString, QVariant> params;
        if (opts.contains("criminal-id")) {
            sql += " AND criminal_id = :criminalId";
            params["criminalId"] = opts["criminal-id"].toInt();
        }
        if (opts.contains("type")) {
            sql += " AND record_type = :recordType";
            params["recordType"] = recordTypeFilter(opts["type"]);
        }
        if (opts.contains("status")) {
            sql += " AND status = :status";
            params["status"] = recordStatusFilter(opts["status"]);
        }
        sql += " ORDER BY id DESC LIMIT :limit";
        params["limit"] = limit;

        auto rows = db.select(sql, params);
        if (rows.isEmpty()) {
            std::cout << "（无数据）\n";
            return 0;
        }
        std::cout << Formatter::toTable(rows, RECORD_LIST_HEADERS).toUtf8().constData();
        return 0;

    } else if (subCmd == "show") {
        if (positionalArgs.isEmpty()) {
            std::cerr << "错误: show 需要指定 ID\n";
            return 1;
        }
        bool ok = false;
        int id = positionalArgs[0].toInt(&ok);
        if (!ok || id <= 0) {
            std::cerr << "错误: 无效的 ID\n";
            return 1;
        }
        auto row = db.selectOne(
            "SELECT * FROM records WHERE id = :id",
            {{"id", id}}
        );
        if (row.isEmpty()) {
            std::cerr << "错误: 未找到 ID=" << id << " 的记录\n";
            return 1;
        }
        row = decryptRecordRow(row);
        if (fmt == OutputFormat::Json) {
            std::cout << Formatter::toJson(QVector<QMap<QString, QVariant>>{row}, RECORD_DETAIL_FIELDS).toUtf8().constData();
        } else if (fmt == OutputFormat::Yaml) {
            std::cout << Formatter::toYaml(QVector<QMap<QString, QVariant>>{row}, RECORD_DETAIL_FIELDS).toUtf8().constData();
        } else if (fmt == OutputFormat::Csv) {
            std::cout << Formatter::toCsv(QVector<QMap<QString, QVariant>>{row}, RECORD_DETAIL_FIELDS).toUtf8().constData();
        } else {
            std::cout << Formatter::toTable(QVector<QMap<QString, QVariant>>{row}, RECORD_DETAIL_HEADERS).toUtf8().constData();
        }
        return 0;

    } else if (subCmd == "-h" || subCmd == "--help" || subCmd.isEmpty()) {
        printRecordsHelp();
        return 0;
    } else {
        std::cerr << "错误: 未知的子命令 '" << qPrintable(subCmd) << "'\n";
        printRecordsHelp();
        return 1;
    }
}
