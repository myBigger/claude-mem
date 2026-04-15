#include "models/Record.h"
#include <QMap>
QString Record::typeToString(RecordType t) {
    switch(t){case RecordType::EntryNote:return "入监笔录";case RecordType::DailyTalk:return "日常谈话";
    case RecordType::CaseInvestigation:return "案件调查";case RecordType::AdminPenalty:return "行政处罚告知";
    case RecordType::SentenceEvaluate:return "减刑假释评估";case RecordType::ExitEducation:return "出监教育";
    case RecordType::MedicalCheck:return "医疗检查";case RecordType::FamilyVisit:return "亲属会见";
    default:return "自定义";}
}
QString Record::statusToString(RecordStatus s) {
    switch(s){case RecordStatus::Draft:return "草稿";case RecordStatus::PendingApproval:return "待审批";
    case RecordStatus::InApproval:return "审批中";case RecordStatus::Approved:return "已通过";
    default:return "已驳回";}
}
QString Record::typeToCode(RecordType t) {
    static QMap<RecordType,QString> m = {{RecordType::EntryNote,"RT-01"},{RecordType::DailyTalk,"RT-02"},
        {RecordType::CaseInvestigation,"RT-03"},{RecordType::AdminPenalty,"RT-04"},
        {RecordType::SentenceEvaluate,"RT-05"},{RecordType::ExitEducation,"RT-06"},
        {RecordType::MedicalCheck,"RT-07"},{RecordType::FamilyVisit,"RT-08"},
        {RecordType::Custom,"RT-99"}};
    return m.value(t,"RT-99");
}
