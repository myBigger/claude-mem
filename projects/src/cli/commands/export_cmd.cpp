#include "export_cmd.h"
#include "../formatter.h"
#include "../cli_utils.h"
#include <database/DatabaseManager.h>
#include <iostream>

static void printExportHelp() {
    std::cout << "用法: prison-cli export <类型> [选项]\n\n";
    std::cout << "类型:\n";
    std::cout << "  criminals   导出服刑人员数据\n";
    std::cout << "  records     导出档案记录数据\n";
    std::cout << "  all         导出全部数据\n\n";
    std::cout << "选项:\n";
    std::cout << "  --format <fmt>   输出格式: csv(默认) json yaml\n";
    std::cout << "  --limit <n>      限制结果数量，默认10000\n";
    std::cout << "\n示例:\n";
    std::cout << "  prison-cli export criminals --format=csv > criminals.csv\n";
    std::cout << "  prison-cli export records --format=json > records.json\n";
}

static const QStringList CRIMINAL_EXPORT_FIELDS = {
    "id", "criminal_id", "name", "gender", "ethnicity", "birth_date",
    "id_card_number", "native_place", "education", "crime",
    "sentence_years", "sentence_months", "entry_date", "original_court",
    "district", "cell", "crime_type", "manage_level", "handler_id",
    "remark", "archived", "created_at", "updated_at"
};

static const QStringList RECORD_EXPORT_FIELDS = {
    "id", "record_id", "record_type", "criminal_id", "criminal_name",
    "record_date", "record_location", "interrogator_id", "recorder_id",
    "present_persons", "content", "signed_interrogator", "signed_recorder",
    "signed_subject", "status", "approver1_id", "approver1_opinion",
    "approver1_result", "approver2_id", "approver2_opinion", "approver2_result",
    "reject_reason", "version", "created_by", "created_at", "updated_at"
};

int ExportCmd::run(const QStringList& args) {
    QString exportType;
    QMap<QString, QString> opts;

    for (int i = 0; i < args.size(); ++i) {
        const QString& arg = args[i];
        if (!arg.startsWith('-') && exportType.isEmpty()) {
            exportType = arg;
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

    if (exportType.isEmpty()) {
        printExportHelp();
        return 1;
    }

    QString formatStr = opts.value("format");
    if (formatStr.isEmpty()) formatStr = "csv";
    OutputFormat fmt = Formatter::parseFormat(formatStr);

    int limit = opts.value("limit").toInt();
    if (limit <= 0) limit = 10000;

    auto& db = DatabaseManager::instance();
    QString out;

    if (exportType == "criminals") {
        auto rows = db.select(
            "SELECT * FROM criminals ORDER BY id DESC LIMIT :limit",
            {{"limit", limit}}
        );
        out = Formatter::format(rows, CRIMINAL_EXPORT_FIELDS, fmt);

    } else if (exportType == "records") {
        auto rows = db.select(
            "SELECT * FROM records ORDER BY id DESC LIMIT :limit",
            {{"limit", limit}}
        );
        for (auto& row : rows) {
            if (row.contains("content")) {
                row["content"] = CliUtils::decryptField(row.value("content").toString());
            }
        }
        out = Formatter::format(rows, RECORD_EXPORT_FIELDS, fmt);

    } else if (exportType == "all") {
        auto criminals = db.select(
            "SELECT * FROM criminals ORDER BY id DESC LIMIT :limit",
            {{"limit", limit}}
        );
        auto records = db.select(
            "SELECT * FROM records ORDER BY id DESC LIMIT :limit",
            {{"limit", limit}}
        );
        for (auto& row : records) {
            if (row.contains("content")) {
                row["content"] = CliUtils::decryptField(row.value("content").toString());
            }
        }

        QString criminalsOut = Formatter::format(criminals, CRIMINAL_EXPORT_FIELDS, fmt);
        QString recordsOut = Formatter::format(records, RECORD_EXPORT_FIELDS, fmt);

        out = "===== CRIMINALS (" + QString::number(criminals.size()) + " records) =====\n";
        out += criminalsOut;
        out += "\n===== RECORDS (" + QString::number(records.size()) + " records) =====\n";
        out += recordsOut;

    } else {
        std::cerr << "错误: 未知的导出类型 '" << qPrintable(exportType) << "'\n";
        printExportHelp();
        return 1;
    }

    std::cout << out.toUtf8().constData();
    return 0;
}
