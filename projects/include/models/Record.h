#ifndef RECORD_H
#define RECORD_H
#include <QString>
enum class RecordType { EntryNote, DailyTalk, CaseInvestigation, AdminPenalty, SentenceEvaluate, ExitEducation, MedicalCheck, FamilyVisit, Custom };
enum class RecordStatus { Draft, PendingApproval, InApproval, Approved, Rejected };
struct Record {
    int id = 0; QString recordId; RecordType recordType = RecordType::EntryNote;
    int criminalId = 0; QString criminalName;
    QString recordDate, recordLocation, interrogatorId, recorderId, presentPersons;
    QString content; bool signedInterrogator = false, signedRecorder = false, signedSubject = false;
    RecordStatus status = RecordStatus::Draft;
    int approver1Id = 0, approver2Id = 0;
    QString approver1Opinion, approver2Opinion, approver1Result, approver2Result, rejectReason;
    int version = 1; int createdBy = 0; QString createdAt, updatedAt;
    static QString typeToString(RecordType t);
    static QString statusToString(RecordStatus s);
    static QString typeToCode(RecordType t);
};
#endif
