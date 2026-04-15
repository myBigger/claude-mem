#include "criminals_cmd.h"
#include "../formatter.h"
#include "../cli_utils.h"
#include <database/DatabaseManager.h>
#include <iostream>

static void printCriminalsHelp() {
    std::cout << "用法: prison-cli criminals <子命令> [选项]\n\n";
    std::cout << "子命令:\n";
    std::cout << "  list      列出服刑人员\n";
    std::cout << "  search    搜索服刑人员\n";
    std::cout << "  show      显示详情\n\n";
    std::cout << "选项:\n";
    std::cout << "  --district <name>    按监区筛选\n";
    std::cout << "  --archived <0|1>     显示已归档(1)或未归档(0)，默认0\n";
    std::cout << "  --criminal-name <n>  搜索姓名\n";
    std::cout << "  --criminal-id <id>   服刑人员编号\n";
    std::cout << "  --format <fmt>      输出格式: table(默认) json yaml csv\n";
    std::cout << "  --limit <n>         限制结果数量，默认100\n";
}

static const QStringList CRIMINAL_LIST_FIELDS = {
    "id", "criminal_id", "name", "gender", "district", "cell",
    "crime", "sentence_years", "manage_level", "entry_date", "archived"
};

static const QMap<QString, QString> CRIMINAL_LIST_HEADERS = {
    {"id", "ID"}, {"criminal_id", "编号"}, {"name", "姓名"}, {"gender", "性别"},
    {"district", "监区"}, {"cell", "监室"}, {"crime", "罪名"},
    {"sentence_years", "刑期(年)"}, {"manage_level", "管理等级"},
    {"entry_date", "入狱日期"}, {"archived", "已归档"}
};

static const QStringList CRIMINAL_DETAIL_FIELDS = {
    "id", "criminal_id", "name", "gender", "ethnicity", "birth_date",
    "id_card_number", "native_place", "education", "crime",
    "sentence_years", "sentence_months", "entry_date", "original_court",
    "district", "cell", "crime_type", "manage_level", "handler_id",
    "remark", "archived", "created_at", "updated_at"
};

static const QMap<QString, QString> CRIMINAL_DETAIL_HEADERS = {
    {"id", "ID"}, {"criminal_id", "编号"}, {"name", "姓名"}, {"gender", "性别"},
    {"ethnicity", "民族"}, {"birth_date", "出生日期"}, {"id_card_number", "身份证号"},
    {"native_place", "籍贯"}, {"education", "学历"}, {"crime", "罪名"},
    {"sentence_years", "刑期(年)"}, {"sentence_months", "刑期(月)"},
    {"entry_date", "入狱日期"}, {"original_court", "原判法院"}, {"district", "监区"},
    {"cell", "监室"}, {"crime_type", "案件类型"}, {"manage_level", "管理等级"},
    {"handler_id", "主管警员"}, {"remark", "备注"},
    {"archived", "已归档"}, {"created_at", "创建时间"}, {"updated_at", "更新时间"}
};

static OutputFormat parseFormat(const QStringList& args, int& i) {
    QString v = (i + 1 < args.size()) ? args[i + 1] : "";
    if (!v.startsWith('-')) { ++i; return Formatter::parseFormat(v); }
    return OutputFormat::Table;
}

// Manual parse of remaining args, return {subCmd, positionalArgs}
static QStringList collectPositionalArgs(const QStringList& args, int start) {
    return args.mid(start);
}

int CriminalsCmd::run(const QStringList& args) {
    // Find subcommand (first non-option arg)
    QString subCmd;
    QMap<QString, QString> opts;  // option name → value
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

    // Also parse options in positionalArgs (e.g. --format after subcommand)
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

    // Apply defaults for options not set
    if (!opts.contains("format")) opts["format"] = "table";
    if (!opts.contains("limit")) opts["limit"] = "100";

    OutputFormat fmt = Formatter::parseFormat(opts.value("format"));
    int limit = opts.value("limit").toInt();
    if (limit <= 0) limit = 100;

    auto& db = DatabaseManager::instance();

    if (subCmd == "list") {
        QString sql = "SELECT * FROM criminals WHERE 1=1";
        QMap<QString, QVariant> params;
        if (opts.contains("district")) {
            sql += " AND district LIKE :district";
            params["district"] = "%" + opts["district"] + "%";
        }
        if (opts.contains("archived")) {
            sql += " AND archived = :archived";
            params["archived"] = opts["archived"].toInt();
        } else {
            sql += " AND archived = 0";
        }
        sql += " ORDER BY id DESC LIMIT :limit";
        params["limit"] = limit;

        auto rows = db.select(sql, params);
        if (rows.isEmpty()) {
            std::cout << "（无数据）\n";
            return 0;
        }
        std::cout << Formatter::toTable(rows, CRIMINAL_LIST_HEADERS).toUtf8().constData();
        return 0;

    } else if (subCmd == "search") {
        QString sql = "SELECT * FROM criminals WHERE 1=1";
        QMap<QString, QVariant> params;
        if (opts.contains("criminal-name")) {
            sql += " AND name LIKE :name";
            params["name"] = "%" + opts["criminal-name"] + "%";
        }
        if (opts.contains("district")) {
            sql += " AND district LIKE :district";
            params["district"] = "%" + opts["district"] + "%";
        }
        if (!opts.contains("archived")) {
            sql += " AND archived = 0";
        } else {
            sql += " AND archived = :archived";
            params["archived"] = opts["archived"].toInt();
        }
        sql += " ORDER BY id DESC LIMIT :limit";
        params["limit"] = limit;

        auto rows = db.select(sql, params);
        if (rows.isEmpty()) {
            std::cout << "（无匹配数据）\n";
            return 0;
        }
        std::cout << Formatter::toTable(rows, CRIMINAL_LIST_HEADERS).toUtf8().constData();
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
            "SELECT * FROM criminals WHERE id = :id",
            {{"id", id}}
        );
        if (row.isEmpty()) {
            std::cerr << "错误: 未找到 ID=" << id << " 的记录\n";
            return 1;
        }
        if (fmt == OutputFormat::Json) {
            std::cout << Formatter::toJson(QVector<QMap<QString, QVariant>>{row}, CRIMINAL_DETAIL_FIELDS).toUtf8().constData();
        } else if (fmt == OutputFormat::Yaml) {
            std::cout << Formatter::toYaml(QVector<QMap<QString, QVariant>>{row}, CRIMINAL_DETAIL_FIELDS).toUtf8().constData();
        } else if (fmt == OutputFormat::Csv) {
            std::cout << Formatter::toCsv(QVector<QMap<QString, QVariant>>{row}, CRIMINAL_DETAIL_FIELDS).toUtf8().constData();
        } else {
            std::cout << Formatter::toTable(QVector<QMap<QString, QVariant>>{row}, CRIMINAL_DETAIL_HEADERS).toUtf8().constData();
        }
        return 0;

    } else if (subCmd == "-h" || subCmd == "--help" || subCmd.isEmpty()) {
        printCriminalsHelp();
        return 0;
    } else {
        std::cerr << "错误: 未知的子命令 '" << qPrintable(subCmd) << "'\n";
        printCriminalsHelp();
        return 1;
    }
}
