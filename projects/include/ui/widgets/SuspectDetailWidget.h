#pragma once
#ifndef SUSPECTDETAILWIDGET_H
#define SUSPECTDETAILWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QLabel>
#include "models/Suspect.h"

class SuspectDetailWidget : public QWidget {
    Q_OBJECT

public:
    explicit SuspectDetailWidget(int suspectId = -1, int prefillCaseId = -1,
                                   QWidget* parent = nullptr);
    ~SuspectDetailWidget();

    void loadSuspect(int suspectId);
    void setReadOnly(bool readOnly);

signals:
    void suspectSaved(const Suspect& suspect);
    void suspectDeleted(int suspectId);
    void requestClose();

private slots:
    void onSaveClicked();
    void onDeleteClicked();
    void onCancelClicked();
    void onSelectPhotoClicked();

private:
    void setupUi();
    void populateFields();
    void clearFields();
    bool validateInput();

    int m_suspectId;
    int m_prefillCaseId;
    bool m_isNew;
    bool m_readOnly;
    Suspect m_suspect;

    QLineEdit* m_nameEdit;
    QLineEdit* m_idNumberEdit;
    QDateEdit* m_birthDateEdit;
    QComboBox* m_genderCombo;
    QLineEdit* m_ethnicityEdit;
    QLineEdit* m_nativePlaceEdit;
    QDateEdit* m_custodyDateEdit;
    QLineEdit* m_custodyLocationEdit;
    QLineEdit* m_caseIdEdit;
    QLineEdit* m_photoPathEdit;
    QLabel* m_photoLabel;
    QPushButton* m_selectPhotoBtn;
    QLineEdit* m_fingerprintIdEdit;
    QTextEdit* m_physicalMarksEdit;

    QPushButton* m_saveBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_cancelBtn;

    QLabel* m_titleLabel;
};

#endif // SUSPECTDETAILWIDGET_H
