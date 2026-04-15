#pragma once
#ifndef SUSPECT_H
#define SUSPECT_H

#include <QString>
#include <QDate>

class Suspect {
public:
    enum class Gender {
        Male,
        Female,
        Other
    };

    Suspect();
    Suspect(int id, const QString& name, const QString& idNumber,
            const QDate& birthDate, Gender gender, const QString& ethnicity,
            const QString& nativePlace, const QDate& custodyDate,
            const QString& custodyLocation, int caseId);

    int id() const;
    void setId(int id);

    QString name() const;
    void setName(const QString& name);

    QString idNumber() const;
    void setIdNumber(const QString& idNum);

    QDate birthDate() const;
    void setBirthDate(const QDate& date);

    Gender gender() const;
    void setGender(Gender gender);
    QString genderString() const;
    void setGenderFromString(const QString& genderStr);

    QString ethnicity() const;
    void setEthnicity(const QString& eth);

    QString nativePlace() const;
    void setNativePlace(const QString& place);

    QDate custodyDate() const;
    void setCustodyDate(const QDate& date);

    QString custodyLocation() const;
    void setCustodyLocation(const QString& loc);

    int caseId() const;
    void setCaseId(int caseId);

    QString photoPath() const;
    void setPhotoPath(const QString& path);

    QString fingerprintId() const;
    void setFingerprintId(const QString& id);

    QString physicalMarks() const;
    void setPhysicalMarks(const QString& marks);

    int createdBy() const;
    void setCreatedBy(int userId);

    QDateTime createdAt() const;
    void setCreatedAt(const QDateTime& dt);

    static QString genderDisplayText(Gender g);

private:
    int m_id;
    QString m_name;
    QString m_idNumber;
    QDate m_birthDate;
    Gender m_gender;
    QString m_ethnicity;
    QString m_nativePlace;
    QDate m_custodyDate;
    QString m_custodyLocation;
    int m_caseId;
    QString m_photoPath;
    QString m_fingerprintId;
    QString m_physicalMarks;
    int m_createdBy;
    QDateTime m_createdAt;
};

#endif // SUSPECT_H
