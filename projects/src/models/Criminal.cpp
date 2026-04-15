#include "models/Criminal.h"
QString Criminal::sentenceDisplay() const {
    if (sentenceYears > 0 && sentenceMonths > 0) return QString("%1年%2个月").arg(sentenceYears).arg(sentenceMonths);
    else if (sentenceYears > 0) return QString("%1年").arg(sentenceYears);
    else if (sentenceMonths > 0) return QString("%1个月").arg(sentenceMonths);
    return "未知";
}
