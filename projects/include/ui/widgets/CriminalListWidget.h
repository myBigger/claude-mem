#ifndef CRIMINALLISTWIDGET_H
#define CRIMINALLISTWIDGET_H
#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
class CriminalListWidget : public QWidget {
    Q_OBJECT
public:
    explicit CriminalListWidget(QWidget* parent = nullptr);
    ~CriminalListWidget();
private slots:
    void onSearch(); void onAdd(); void onFetchFromApi(); void onEdit(); void onView();
    void onDelete(); void onExport(); void onSelectionChanged();
    void loadData(const QString& kw="", int page=1);
    int totalPages();
    bool isCurrentUserHandler();
private:
    void setupUI();
    QTableView* m_table = nullptr;
    QStandardItemModel* m_model = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QComboBox *m_districtCombo = nullptr, *m_archiveCombo = nullptr;
    QPushButton *m_addBtn=nullptr, *m_fetchBtn=nullptr, *m_editBtn=nullptr, *m_viewBtn=nullptr, *m_delBtn=nullptr, *m_exportBtn=nullptr;
    QLabel* m_countLabel = nullptr;
    QLabel* m_pageLabel = nullptr;
    QPushButton *m_prevBtn=nullptr, *m_nextBtn=nullptr;
    int m_page = 1, m_pageSize = 20, m_total = 0;
};
#endif
