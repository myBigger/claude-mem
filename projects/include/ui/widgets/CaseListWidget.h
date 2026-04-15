#pragma once
#ifndef CASELISTWIDGET_H
#define CASELISTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QGroupBox>
#include <QString>
#include "models/Case.h"

class CaseListWidget : public QWidget {
    Q_OBJECT

public:
    explicit CaseListWidget(QWidget* parent = nullptr);
    ~CaseListWidget();

    void refresh();

signals:
    void caseSelected(const Case& c);
    void caseCreated(int id);
    void caseUpdated(int id);

private slots:
    void onSearchClicked();
    void onNewClicked();
    void onOpenCase();
    void onEditCase();
    void onDeleteCase();
    void onExportClicked();
    void onRefreshClicked();
    void onCaseContextMenu(const QPoint& pos);

private:
    void setupUi();
    void loadCases(const QString& filter = QString());
    Case getCaseFromRow(int row) const;

    QTableWidget* m_table;
    QLineEdit* m_searchEdit;
    QComboBox* m_statusFilter;
    QComboBox* m_typeFilter;
    QDateEdit* m_startDateFilter;
    QDateEdit* m_endDateFilter;
    QPushButton* m_searchBtn;
    QPushButton* m_newBtn;
    QPushButton* m_exportBtn;
    QLabel* m_countLabel;
    int m_selectedCaseId;

    enum TableColumn {
        ColId = 0,
        ColCaseNumber = 1,
        ColCaseName = 2,
        ColCaseType = 3,
        ColStatus = 4,
        ColFilingDate = 5,
        ColHandler = 6,
        ColUpdated = 7,
        ColAction = 8
    };
};

#endif // CASELISTWIDGET_H
