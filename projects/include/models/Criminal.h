#ifndef CRIMINAL_H
#define CRIMINAL_H
#include <QString>
#include <QDateTime>
struct Criminal {
    int id = 0; QString criminalId, name, gender, ethnicity, birthDate;
    QString idCardNumber, nativePlace, education, crime;
    int sentenceYears = 0, sentenceMonths = 0;
    QString entryDate, originalCourt, district, cell;
    QString crimeType, manageLevel, handlerId, photoPath, remark;
    bool archived = false;
    QDateTime createdAt, updatedAt;
    QString sentenceDisplay() const;
};
#endif
