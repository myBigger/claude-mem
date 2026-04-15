#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QStandardPaths>
#include <iostream>
#include "cli_utils.h"
#include "formatter.h"
#include "commands/criminals_cmd.h"
#include "commands/records_cmd.h"
#include "commands/approval_cmd.h"
#include "commands/audit_cmd.h"
#include "commands/export_cmd.h"

static void printVersion() {
    std::cout << "prison-cli v1.0.0\n";
}

static void printGlobalHelp() {
    std::cout << "用法: prison-cli <子命令> [选项]\n\n";
    std::cout << "子命令:\n";
    std::cout << "  criminals    服刑人员管理\n";
    std::cout << "  records      档案记录管理\n";
    std::cout << "  approval     审批管理\n";
    std::cout << "  audit        审计日志查询\n";
    std::cout << "  export       数据导出\n";
    std::cout << "\n全局选项:\n";
    std::cout << "  --db <path>      指定数据库路径\n";
    std::cout << "  --pin <code>     解锁加密内容（PIN码）\n";
    std::cout << "  -h, --help       显示帮助\n";
    std::cout << "  -v, --version    显示版本\n";
    std::cout << "\n使用 \"prison-cli <子命令> --help\" 查看子命令详细用法。\n";
    std::cout << "\n示例:\n";
    std::cout << "  prison-cli criminals list --district=一监区\n";
    std::cout << "  prison-cli criminals list --district=一监区 --format=json\n";
    std::cout << "  prison-cli records list --criminal-id=1 --format=yaml\n";
    std::cout << "  prison-cli records show 5 --pin=123456\n";
    std::cout << "  prison-cli approval list --status=pending\n";
    std::cout << "  prison-cli audit query --username=admin --limit=50\n";
    std::cout << "  prison-cli export criminals --format=csv > criminals.csv\n";
}

// Parse a QStringList of raw args manually, separating global options from subcommand+args
static void splitGlobalAndCommand(const QStringList& allArgs,
                                  QStringList& globalOpts,
                                  QString& subCmd,
                                  QStringList& subCmdArgs) {
    // Skip argv[0] (app name)
    QStringList args = allArgs.mid(1);
    for (int i = 0; i < args.size(); ++i) {
        const QString& arg = args[i];
        // Subcommand: first non-option argument
        if (!arg.startsWith('-') && subCmd.isEmpty()) {
            subCmd = arg;
            // Everything after subcommand is subcommand args
            subCmdArgs = args.mid(i + 1);
            break;
        } else if (arg.startsWith('-')) {
            globalOpts.append(arg);
            // If option has inline value (--foo=bar), no extra value
            if (!arg.contains('=')) {
                // Check if next arg is a value (not an option)
                if (i + 1 < args.size() && !args[i + 1].startsWith('-')) {
                    globalOpts.append(args[i + 1]);
                    ++i;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName("prison-cli");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("PrisonSystem");
    QCoreApplication app(argc, argv);

    // Parse raw argv directly, bypassing QCoreApplication's argument processing
    QStringList rawArgs;
    for (int i = 0; i < argc; ++i) {
        rawArgs << QString::fromLocal8Bit(argv[i]);
    }

    // Parse global options manually
    QString dbPath;
    QString pin;
    bool showHelp = false;
    bool showVersion = false;
    QString subCmd;
    QStringList subCmdArgs;

    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "-h" || arg == "--help" || arg == "-help" || arg == "/?") {
            showHelp = true;
        } else if (arg == "-v" || arg == "--version" || arg == "-version") {
            showVersion = true;
        } else if (arg == "-d" || arg == "--db") {
            if (i + 1 < argc) dbPath = QString::fromLocal8Bit(argv[++i]);
        } else if (arg.startsWith("--db=")) {
            dbPath = arg.mid(5);
        } else if (arg == "-p" || arg == "--pin") {
            if (i + 1 < argc) pin = QString::fromLocal8Bit(argv[++i]);
        } else if (arg.startsWith("--pin=")) {
            pin = arg.mid(6);
        } else if (!arg.startsWith('-') && subCmd.isEmpty()) {
            subCmd = arg;
            // Collect remaining args as subcommand args
            while (++i < argc) {
                subCmdArgs << QString::fromLocal8Bit(argv[i]);
            }
            break;
        }
    }

    if (showVersion) {
        printVersion();
        return 0;
    }
    if (showHelp || subCmd.isEmpty()) {
        printGlobalHelp();
        return showHelp ? 0 : 1;
    }

    // Also check subCmdArgs for global options (they may appear after subcommand)
    QStringList remainingSubCmdArgs;
    for (int i = 0; i < subCmdArgs.size(); ++i) {
        const QString& arg = subCmdArgs[i];
        if (arg == "-d" || arg == "--db") {
            if (i + 1 < subCmdArgs.size()) dbPath = subCmdArgs[++i];
        } else if (arg.startsWith("--db=")) {
            dbPath = arg.mid(5);
        } else if (arg == "-p" || arg == "--pin") {
            if (i + 1 < subCmdArgs.size()) pin = subCmdArgs[++i];
        } else if (arg.startsWith("--pin=")) {
            pin = arg.mid(6);
        } else if (arg == "-h" || arg == "--help") {
            showHelp = true;
        } else if (arg == "-v" || arg == "--version") {
            showVersion = true;
        } else {
            remainingSubCmdArgs.append(arg);
        }
    }
    subCmdArgs = remainingSubCmdArgs;

    if (showVersion) {
        printVersion();
        return 0;
    }
    if (showHelp) {
        printGlobalHelp();
        return 0;
    }

    // Find database
    if (dbPath.isEmpty()) {
        dbPath = CliUtils::findDatabasePath();
    }
    if (dbPath.isEmpty()) {
        std::cerr << "错误: 未找到数据库文件。请使用 --db 指定路径。\n";
        return 1;
    }
    if (!CliUtils::initDatabase(dbPath)) {
        return 1;
    }

    // Unlock encryption if PIN provided
    if (!pin.isEmpty()) {
        if (!CliUtils::unlockEncryption(pin)) {
            std::cerr << "错误: PIN 错误或加密未初始化。\n";
            return 1;
        }
    }

    // Route to subcommand
    int exitCode = 0;
    if (subCmd == "criminals") {
        exitCode = CriminalsCmd::run(subCmdArgs);
    } else if (subCmd == "records") {
        exitCode = RecordsCmd::run(subCmdArgs);
    } else if (subCmd == "approval") {
        exitCode = ApprovalCmd::run(subCmdArgs);
    } else if (subCmd == "audit") {
        exitCode = AuditCmd::run(subCmdArgs);
    } else if (subCmd == "export") {
        exitCode = ExportCmd::run(subCmdArgs);
    } else {
        std::cerr << "错误: 未知子命令 '" << qPrintable(subCmd) << "'\n";
        printGlobalHelp();
        exitCode = 1;
    }

    return exitCode;
}
