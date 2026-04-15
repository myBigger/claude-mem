#include "models/Case.h"

const QString Case::STATUS_INVESTIGATION = "investigation";
const QString Case::STATUS_PENDING_TRIAL = "pending_trial";
const QString Case::STATUS_CLOSED = "closed";

const QStringList Case::CASE_TYPES = {
    "刑事案件",
    "行政案件",
    "民事案件",
    "执行案件",
    "申诉案件",
    "其他"
};

Case::Case()
    : id(0)
    , status(STATUS_INVESTIGATION)
    , createdBy(0)
{}

Case::Case(int id)
    : id(id)
    , status(STATUS_INVESTIGATION)
    , createdBy(0)
{}

QString Case::statusDisplayName() const
{
    if (status == STATUS_INVESTIGATION) return "侦查中";
    if (status == STATUS_PENDING_TRIAL) return "审查起诉";
    if (status == STATUS_CLOSED) return "已结案";
    return status;
}
