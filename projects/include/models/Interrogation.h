#pragma once
#ifndef INTERROGATION_H
#define INTERROGATION_H

#include <QString>
#include <QDateTime>
#include <QDate>

class Interrogation {
public:
    enum class RecordType {
        Interrogation,  // 讯问笔录
        Inquiry,        // 询问笔录
        Identification  // 辨认笔录
    };

    enum class Status {
        Draft,          // 草稿
        PendingReview,  // 待审核
        Approved,       // 已审核
        Archived        // 已归档
    };

    struct QaEntry {
        bool isQuestion; // true = 问, false = 答
        QString speaker; // 说话人/角色
        QString content; // 内容
        QDateTime timestamp;
    };

    Interrogation();
    Interrogation(int id, int caseId, int suspectId, RecordType type,
                   const QString& recordNumber, const QDate& recordDate,
                   const QString& location, const QString& interrogator,
                   const QString& recorder, const QString& content, Status status);

    int id() const;
    void setId(int id);

    int caseId() const;
    void setCaseId(int caseId);

    int suspectId() const;
    void setSuspectId(int suspectId);

    RecordType recordType() const;
    void setRecordType(RecordType type);
    QString recordTypeString() const;
    void setRecordTypeFromString(const QString& typeStr);

    QString recordNumber() const;
    void setRecordNumber(const QString& num);

    QDate recordDate() const;
    void setRecordDate(const QDate& date);

    QString recordLocation() const;
    void setRecordLocation(const QString& loc);

    QString interrogator() const;
    void setInterrogator(const QString& name);

    QString recorder() const;
    void setRecorder(const QString& name);

    QString content() const;
    void setContent(const QString& content);

    Status status() const;
    void setStatus(Status status);
    QString statusString() const;
    void setStatusFromString(const QString& statusStr);

    int approvedBy() const;
    void setApprovedBy(int userId);

    QDateTime approvedAt() const;
    void setApprovedAt(const QDateTime& dt);

    int createdBy() const;
    void setCreatedBy(int userId);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dt);

    QDateTime updatedAt() const;
    void setUpdatedAt(const QDateTime& dt);

    static QString recordTypeDisplayText(RecordType t);
    static QString statusDisplayText(Status s);

    // 序列化：文本内容 <-> 结构化
    static QString serializeContent(const QList<QaEntry>& entries);
    static QList<QaEntry> deserializeContent(const QString& content);

private:
    int m_id;
    int m_caseId;
    int m_suspectId;
    RecordType m_recordType;
    QString m_recordNumber;
    QDate m_recordDate;
    QString m_recordLocation;
    QString m_interrogator;
    QString m_recorder;
    QString m_content;
    Status m_status;
    int m_approvedBy;
    QDateTime m_approvedAt;
    int m_createdBy;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

#endif // INTERROGATION_H
