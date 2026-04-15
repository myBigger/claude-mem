#ifndef FORMATTER_H
#define FORMATTER_H
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>

enum class OutputFormat { Table, Json, Yaml, Csv };

class Formatter {
public:
    static OutputFormat parseFormat(const QString& fmt);

    // 通用数据格式化
    static QString format(const QVector<QMap<QString, QVariant>>& rows,
                          const QStringList& fields,
                          OutputFormat fmt);

    static QString formatSingle(const QMap<QString, QVariant>& row,
                                const QStringList& fields,
                                OutputFormat fmt);

    // 各格式独立方法（供 export 复用）
    static QString toTable(const QVector<QMap<QString, QVariant>>& rows,
                           const QStringList& fields,
                           const QStringList& headers);

    static QString toJson(const QVector<QMap<QString, QVariant>>& rows,
                           const QStringList& fields);

    static QString toYaml(const QVector<QMap<QString, QVariant>>& rows,
                           const QStringList& fields);

    static QString toCsv(const QVector<QMap<QString, QVariant>>& rows,
                          const QStringList& fields);

    // 带中文表头版本的 table
    static QString toTable(const QVector<QMap<QString, QVariant>>& rows,
                           const QMap<QString, QString>& fieldHeaders);

private:
    static QString escapeCsv(const QString& value);
    static QString variantToString(const QVariant& v);
};

#endif // FORMATTER_H
