#ifndef CASE_H
#define CASE_H

#include <QString>
#include <QDate>

/**
 * @brief 案件数据模型
 */
struct Case {
    int id;
    QString caseNumber;      // 案号
    QString caseName;         // 案件名称
    QString caseType;         // 案件类型
    QString status;           // 状态: investigation/pending_trial/closed
    QDate filingDate;        // 立案日期
    QString handler;          // 承办人
    QString summary;          // 摘要
    int createdBy;            // 创建人ID
    QString createdAt;        // 创建时间
    QString updatedAt;        // 更新时间

    // 状态常量
    static const QString STATUS_INVESTIGATION;
    static const QString STATUS_PENDING_TRIAL;
    static const QString STATUS_CLOSED;

    // 类型常量
    static const QStringList CASE_TYPES;

    Case();
    Case(int id);

    bool isValid() const { return id > 0; }
    QString statusDisplayName() const;
};

#endif // CASE_H
