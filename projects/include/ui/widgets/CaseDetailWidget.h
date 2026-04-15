#pragma once
#ifndef CASEDETAILWIDGET_H
#define CASEDETAILWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include "models/Case.h"

class CaseDetailWidget : public QWidget {
    Q_OBJECT

public:
    explicit CaseDetailWidget(int caseId = -1, QWidget* parent = nullptr);
    ~CaseDetailWidget();

    void loadCase(int caseId);
    void setReadOnly(bool readOnly);

signals:
    void caseSaved(const Case& caseObj);
    void caseDeleted(int caseId);
    void requestClose();

private slots:
    void onSaveClicked();
    void onDeleteClicked();
    void onCancelClicked();

private:
    void setupUi();
    void populateFields();
    void clearFields();
    bool validateInput();

    int m_caseId;
    bool m_isNew;
    bool m_readOnly;
    Case m_case;

    // Form fields
    QLineEdit* m_caseNumberEdit;
    QLineEdit* m_caseNameEdit;
    QComboBox* m_caseTypeCombo;
    QComboBox* m_statusCombo;
    QDateEdit* m_filingDateEdit;
    QLineEdit* m_handlerEdit;
    QTextEdit* m_summaryEdit;

    // Buttons
    QPushButton* m_saveBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_cancelBtn;

    QLabel* m_titleLabel;
};

#endif // CASEDETAILWIDGET_H
