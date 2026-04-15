#include "ui/widgets/RecordThemeSelector.h"
#include "ui/widgets/LinedTextEdit.h"
#include <QButtonGroup>
#include <QFrame>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyle>

// ──────────────────────────────────────────────
// 主题色预览按钮（圆形色块）
// ──────────────────────────────────────────────
class ThemePreviewButton : public QPushButton
{
public:
    ThemePreviewButton(LinedTextEdit::ThemeId id, const RecordTheme& theme, QWidget* parent = nullptr)
        : QPushButton(parent), m_id(id), m_theme(theme)
    {
        setFixedSize(36, 36);
        setToolTip(theme.name);
        setCursor(Qt::PointingHandCursor);
        setCheckable(true);
        setStyleSheet(
            "QPushButton {"
            "  border: 2px solid #CCCCCC;"
            "  border-radius: 18px;"
            "  background-color: " + theme.bgColor.name() + ";"
            "  padding: 0px;"
            "}"
            "QPushButton:hover {"
            "  border: 2px solid #888888;"
            "}"
            "QPushButton:checked {"
            "  border: 3px solid #3366CC;"
            "}"
        );
    }

    LinedTextEdit::ThemeId id() const { return m_id; }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRectF r = rect().adjusted(3, 3, -3, -3);
        p.setBrush(m_theme.bgColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(r);

        // 画三条横线模拟信签纸
        p.setPen(QPen(QColor(m_theme.lineColor), 1.5));
        int cy = int(r.center().y());
        p.drawLine(int(r.left() + 7), cy - 6, int(r.right() - 7), cy - 6);
        p.drawLine(int(r.left() + 7), cy,     int(r.right() - 7), cy);
        p.drawLine(int(r.left() + 7), cy + 6, int(r.right() - 7), cy + 6);
    }

private:
    LinedTextEdit::ThemeId m_id;
    RecordTheme m_theme;
};

// ──────────────────────────────────────────────
// RecordThemeSelector 实现
// ──────────────────────────────────────────────
RecordThemeSelector::RecordThemeSelector(LinedTextEdit* targetEditor, QWidget* parent)
    : QWidget(parent)
    , m_targetEditor(targetEditor)
    , m_themeNameLabel(nullptr)
    , m_currentId(LinedTextEdit::ThemeWhite)
{
    setObjectName("RecordThemeSelector");

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 4, 0, 4);
    mainLayout->setSpacing(8);

    mainLayout->addWidget(new QLabel(QString::fromUtf8("📋 主题：")));

    // 创建主题按钮组
    QButtonGroup* group = new QButtonGroup(this);
    group->setExclusive(true);

    const QVector<RecordTheme>& themes = LinedTextEdit::builtInThemes();
    for (int i = 0; i < themes.size(); ++i) {
        ThemePreviewButton* btn = new ThemePreviewButton(
            LinedTextEdit::ThemeId(i), themes[i], this);
        connect(btn, &QPushButton::clicked, this, [this, btn]() {
            onThemeButtonClicked(btn->id());
        });
        mainLayout->addWidget(btn);
        m_themeButtons.append(btn);
        group->addButton(btn);
    }

    // 默认选中白色主题
    if (!m_themeButtons.isEmpty()) {
        m_themeButtons.first()->setChecked(true);
        const RecordTheme& firstTheme = themes.first();
        m_currentId = LinedTextEdit::ThemeWhite;
        if (m_targetEditor) m_targetEditor->setTheme(LinedTextEdit::ThemeWhite);
        emit themeChanged(LinedTextEdit::ThemeWhite, firstTheme.name);
    }

    m_themeNameLabel = new QLabel();
    m_themeNameLabel->setStyleSheet(
        "QLabel { color: #666666; font-size: 13px; padding-left: 8px; }");
    mainLayout->addWidget(m_themeNameLabel);
    mainLayout->addStretch();

    // 监听主题变化，同步标签
    connect(this, &RecordThemeSelector::themeChanged,
            this, [this](LinedTextEdit::ThemeId, const QString& name) {
                if (m_themeNameLabel) m_themeNameLabel->setText(name);
            });
}

RecordThemeSelector::~RecordThemeSelector() = default;

void RecordThemeSelector::rebuildButtons()
{
    // 按钮已在构造函数中一次性创建，此处仅处理动态切换
    // 如需动态增删主题，可在此扩展
}

void RecordThemeSelector::onThemeButtonClicked(LinedTextEdit::ThemeId id)
{
    if (!m_targetEditor) return;
    m_currentId = id;
    m_targetEditor->setTheme(id);

    const QVector<RecordTheme>& themes = LinedTextEdit::builtInThemes();
    if (id >= 0 && id < themes.size()) {
        emit themeChanged(id, themes.at(id).name);
    }
}
