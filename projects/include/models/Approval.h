#ifndef APPROVAL_H
#define APPROVAL_H
#include <QString>
struct Approval {
    int id=0, recordId=0, approvalLevel=0, approverId=0;
    QString result, opinion, rejectReason, approvalTime, createdAt;
};
#endif
