#include "formatter.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariant>
#include <iostream>

OutputFormat Formatter::parseFormat(const QString& fmt) {
    if (fmt == "json")  return OutputFormat::Json;
    if (fmt == "yaml")  return OutputFormat::Yaml;
    if (fmt == "csv")   return OutputFormat::Csv;
    return OutputFormat::Table;
}

QString Formatter::variantToString(const QVariant& v) {
    if (!v.isValid() || v.isNull()) return QString();
    if (v.typeId() == QVariant::Bool) return v.toBool() ? "是" : "否";
    if (v.typeId() == QVariant::DateTime) {
        return v.toDateTime().toString("yyyy-MM-dd HH:mm:ss");
    }
    if (v.typeId() == QVariant::Date) {
        return v.toDate().toString("yyyy-MM-dd");
    }
    return v.toString();
}

QString Formatter::format(const QVector<QMap<QString, QVariant>>& rows,
                           const QStringList& fields,
                           OutputFormat fmt) {
    switch (fmt) {
        case OutputFormat::Json: return toJson(rows, fields);
        case OutputFormat::Yaml: return toYaml(rows, fields);
        case OutputFormat::Csv:  return toCsv(rows, fields);
        default: {
            QMap<QString, QString> headers;
            for (const QString& f : fields) headers[f] = f;
            return toTable(rows, headers);
        }
    }
}

QString Formatter::formatSingle(const QMap<QString, QVariant>& row,
                                 const QStringList& fields,
                                 OutputFormat fmt) {
    QVector<QMap<QString, QVariant>> rows;
    rows.append(row);
    return format(rows, fields, fmt);
}

QString Formatter::toTable(const QVector<QMap<QString, QVariant>>& rows,
                            const QMap<QString, QString>& fieldHeaders) {
    if (rows.isEmpty()) return "（无数据）\n";

    QStringList fields;
    QStringList headers;
    for (auto it = fieldHeaders.constBegin(); it != fieldHeaders.constEnd(); ++it) {
        fields.append(it.key());
        headers.append(it.value());
    }

    // 计算每列宽度
    QVector<int> colWidths(fields.size(), 0);
    for (int i = 0; i < fields.size(); ++i) {
        colWidths[i] = headers[i].length();
    }
    for (const auto& row : rows) {
        for (int i = 0; i < fields.size(); ++i) {
            int len = variantToString(row.value(fields[i])).length();
            if (len > colWidths[i]) colWidths[i] = len;
        }
    }

    // 表头
    QString line;
    // 分隔线
    for (int i = 0; i < fields.size(); ++i) {
        if (i > 0) line += "  ";
        line += QString(colWidths[i], '-');
    }
    line += "\n";

    // 标题行
    QString titleLine;
    for (int i = 0; i < fields.size(); ++i) {
        if (i > 0) titleLine += "  ";
        titleLine += headers[i].leftJustified(colWidths[i]);
    }
    titleLine += "\n";

    // 数据行
    QString dataLines;
    for (const auto& row : rows) {
        for (int i = 0; i < fields.size(); ++i) {
            if (i > 0) dataLines += "  ";
            dataLines += variantToString(row.value(fields[i])).leftJustified(colWidths[i]);
        }
        dataLines += "\n";
    }

    return titleLine + line + dataLines;
}

QString Formatter::toTable(const QVector<QMap<QString, QVariant>>& rows,
                            const QStringList& fields,
                            const QStringList& headers) {
    QMap<QString, QString> h;
    for (int i = 0; i < fields.size() && i < headers.size(); ++i) {
        h[fields[i]] = headers[i];
    }
    return toTable(rows, h);
}

QString Formatter::toJson(const QVector<QMap<QString, QVariant>>& rows,
                           const QStringList& fields) {
    QJsonArray arr;
    for (const auto& row : rows) {
        QJsonObject obj;
        for (const QString& f : fields) {
            const QVariant& v = row.value(f);
            if (v.typeId() == QVariant::Bool) {
                obj[f] = v.toBool();
            } else if (v.typeId() == QVariant::Int || v.typeId() == QVariant::LongLong) {
                obj[f] = v.toLongLong();
            } else if (v.typeId() == QVariant::Double) {
                obj[f] = v.toDouble();
            } else {
                obj[f] = v.toString();
            }
        }
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString Formatter::escapeCsv(const QString& value) {
    QString v = value;
    if (v.contains(',') || v.contains('"') || v.contains('\n')) {
        v.replace('"', "\"\"");
        v = "\"" + v + "\"";
    }
    return v;
}

QString Formatter::toCsv(const QVector<QMap<QString, QVariant>>& rows,
                           const QStringList& fields) {
    // BOM for Excel UTF-8 compatibility
    QString out = "\ufeff";
    out += fields.join(',') + "\n";
    for (const auto& row : rows) {
        QStringList cells;
        for (const QString& f : fields) {
            cells.append(escapeCsv(variantToString(row.value(f))));
        }
        out += cells.join(',') + "\n";
    }
    return out;
}

QString Formatter::toYaml(const QVector<QMap<QString, QVariant>>& rows,
                            const QStringList& fields) {
    QString out;
    for (int ri = 0; ri < rows.size(); ++ri) {
        const auto& row = rows[ri];
        if (rows.size() > 1) out += "- ";
        for (int fi = 0; fi < fields.size(); ++fi) {
            const QString& f = fields[fi];
            QString val = variantToString(row.value(f));
            if (val.contains('\n') || val.contains(':') || val.contains('#')) {
                val = "\"" + val.replace("\"", "\\\"") + "\"";
            }
            if (fi == 0 && rows.size() > 1) {
                out += f + ": " + val + "\n";
            } else {
                out += "  " + f + ": " + val + "\n";
            }
        }
        if (ri < rows.size() - 1) out += "\n";
    }
    return out;
}
