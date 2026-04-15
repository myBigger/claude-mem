#ifndef LINEDTEXTEDIT_H
#define LINEDTEXTEDIT_H

#include <QTextEdit>
#include <QPainter>
#include <QRectF>
#include <QVector>
#include <QPair>

/**
 * @brief 笔录主题配置
 */
struct RecordTheme {
    QString name;               // 主题名称
    QColor bgColor;              // 背景色
    QColor lineColor;            // 横线颜色
    QColor lineColorLight;       // 次要横线颜色（如需双线）
    int lineSpacing;             // 行间距（像素）
    int lineHeight;              // 横线粗细
    QColor textColor;            // 文字颜色
    QColor selectionColor;       // 选中背景色
    QString fontFamily;          // 字体
};

class LinedTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    // 预设主题 ID
    enum ThemeId {
        ThemeWhite = 0,    // 白色信签纸（默认）
        ThemeYellow = 1,   // 米黄色档案纸
        ThemeGreen  = 2,   // 绿色护眼纸
        ThemeBlue   = 3,   // 蓝色文件纸
        ThemeDark   = 4,   // 深色夜间模式
        ThemeCount
    };
    Q_ENUM(ThemeId)

    explicit LinedTextEdit(QWidget* parent = nullptr);
    ~LinedTextEdit() override;

    // 主题切换
    void setTheme(ThemeId id);
    ThemeId currentTheme() const { return m_themeId; }

    // 获取内置主题列表
    static const QVector<RecordTheme>& builtInThemes();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyTheme(const RecordTheme& theme);
    void drawLinedBackground(QPainter& painter, const QRectF& exposedRect);
    int effectiveLineHeight() const;

    ThemeId m_themeId = ThemeWhite;
    RecordTheme m_currentTheme;
};

#endif // LINEDTEXTEDIT_H
