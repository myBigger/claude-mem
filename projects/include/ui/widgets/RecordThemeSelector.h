#ifndef RECORDTHEMESELECTOR_H
#define RECORDTHEMESELECTOR_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include "LinedTextEdit.h"

/**
 * @brief 笔录编辑器主题选择工具栏
 *        显示各主题预览色块，点击切换
 */
class RecordThemeSelector : public QWidget
{
    Q_OBJECT

public:
    explicit RecordThemeSelector(LinedTextEdit* targetEditor, QWidget* parent = nullptr);
    ~RecordThemeSelector() override;

signals:
    void themeChanged(LinedTextEdit::ThemeId id, const QString& name);

private slots:
    void onThemeButtonClicked(LinedTextEdit::ThemeId id);

private:
    void rebuildButtons();

    LinedTextEdit* m_targetEditor;
    QVector<QPushButton*> m_themeButtons;
    QLabel* m_themeNameLabel;
    LinedTextEdit::ThemeId m_currentId;
};

#endif // RECORDTHEMESELECTOR_H
