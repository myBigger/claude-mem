#include "ui/widgets/LinedTextEdit.h"
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QScrollBar>
#include <QDebug>

// ──────────────────────────────────────────────
// 内置主题定义
// ──────────────────────────────────────────────
static const QVector<RecordTheme>& builtInThemes_()
{
    static const QVector<RecordTheme> themes = {
        // ThemeWhite — 白色信签纸（默认）
        RecordTheme{
            QStringLiteral("白色信签纸"),
            QColor("#FFFFFF"),
            QColor("#C8D8E8"),   // 浅蓝灰横线
            QColor("#E0EBF5"),
            28,                  // 行高（像素）
            1,                   // 线粗
            QColor("#1A1A1A"),
            QColor("#BEE0F5"),
            QStringLiteral("思源宋体")
        },
        // ThemeYellow — 米黄色档案纸（传统）
        RecordTheme{
            QStringLiteral("米黄色档案纸"),
            QColor("#FAF3E0"),
            QColor("#D4B896"),
            QColor("#E8D4A8"),
            30,
            1,
            QColor("#2C1810"),
            QColor("#F5E0B0"),
            QStringLiteral("思源宋体")
        },
        // ThemeGreen — 绿色护眼纸
        RecordTheme{
            QStringLiteral("绿色护眼纸"),
            QColor("#EDF5E5"),
            QColor("#A8C89A"),
            QColor("#D4E8C4"),
            28,
            1,
            QColor("#1A2E1A"),
            QColor("#C8E0B8"),
            QStringLiteral("思源黑体")
        },
        // ThemeBlue — 蓝色文件纸（公检法风格）
        RecordTheme{
            QStringLiteral("蓝色文件纸"),
            QColor("#EEF4FB"),
            QColor("#7BA8D4"),
            QColor("#A8C8E8"),
            30,
            1,
            QColor("#0D1E3A"),
            QColor("#A8C4E8"),
            QStringLiteral("思源宋体")
        },
        // ThemeDark — 深色夜间模式
        RecordTheme{
            QStringLiteral("深色夜间模式"),
            QColor("#1E2330"),
            QColor("#3A4560"),
            QColor("#2A3548"),
            28,
            1,
            QColor("#D4DDE8"),
            QColor("#2A3858"),
            QStringLiteral("思源黑体")
        },
    };
    return themes;
}

const QVector<RecordTheme>& LinedTextEdit::builtInThemes() { return builtInThemes_(); }

// ──────────────────────────────────────────────
// LinedTextEdit 实现
// ──────────────────────────────────────────────
LinedTextEdit::LinedTextEdit(QWidget* parent)
    : QTextEdit(parent)
    , m_themeId(ThemeWhite)
{
    // 初始应用白色信签纸主题
    applyTheme(builtInThemes_().at(ThemeWhite));

    // 文本变化时立即重绘横线背景
    connect(this, &QTextEdit::textChanged, this, [this]() { update(); });
}

LinedTextEdit::~LinedTextEdit() = default;

// ──────────────────────────────────────────────
// 主题切换
// ──────────────────────────────────────────────
void LinedTextEdit::setTheme(ThemeId id)
{
    if (id < 0 || id >= ThemeCount) return;
    m_themeId = id;
    applyTheme(builtInThemes_().at(id));
}

void LinedTextEdit::applyTheme(const RecordTheme& theme)
{
    m_currentTheme = theme;

    // QTextEdit stylesheet
    setStyleSheet(QString(
        "QTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  selection-background-color: %3;"
        "  border: 1px solid %4;"
        "  padding: 8px 12px;"
        "  font-family: \"%5\", \"SimSun\", \"宋体\", serif;"
        "  font-size: 14pt;"
        "}"
    ).arg(theme.bgColor.name(),
          theme.textColor.name(),
          theme.selectionColor.name(),
          theme.lineColorLight.name(),
          theme.fontFamily));

    // 设置文档默认样式（行高）
    QTextDocument* doc = document();
    QTextOption opt = doc->defaultTextOption();
    opt.setAlignment(Qt::AlignLeft);
    doc->setDefaultTextOption(opt);

    // 触发布局重算后刷新
    document()->setPageSize(QSizeF(width() - 16, height() - 16));
    update();
}

// ──────────────────────────────────────────────
// 计算实际行高（像素）
// ──────────────────────────────────────────────
int LinedTextEdit::effectiveLineHeight() const
{
    QTextDocument* doc = document();
    if (!doc || doc->isEmpty()) {
        return m_currentTheme.lineSpacing;
    }

    // 取第一个块的格式作为参考（有实际内容时更准确）
    QTextCursor cursor(doc);
    if (cursor.movePosition(QTextCursor::Start)) {
        QTextBlockFormat fmt = cursor.blockFormat();
        qreal lineHeight = fmt.lineHeight();
        if (lineHeight > 0) {
            // lineHeightType == FixedHeight 时直接用
            if (fmt.lineHeightType() == QTextBlockFormat::FixedHeight) {
                return static_cast<int>(lineHeight);
            }
            // lineHeightType == ProportionalHeight 时按比例计算
            if (fmt.lineHeightType() == QTextBlockFormat::ProportionalHeight) {
                return static_cast<int>(fontMetrics().height() * lineHeight / 100.0);
            }
            // lineHeightType == MinimumHeight / LineHeightHeight
            if (lineHeight > fontMetrics().height()) {
                return static_cast<int>(lineHeight);
            }
        }
    }
    return m_currentTheme.lineSpacing;
}

// ──────────────────────────────────────────────
// 核心绘制：在文字行的 Y 位置画横线
// ──────────────────────────────────────────────
void LinedTextEdit::drawLinedBackground(QPainter& painter, const QRectF& exposedRect)
{
    QTextDocument* doc = document();
    if (!doc) return;

    QAbstractTextDocumentLayout* layout = doc->documentLayout();
    const int scrollY = verticalScrollBar()->value();
    const int lineH   = effectiveLineHeight();

    // 绘制顶部"装订线"区域（左侧红色竖线，模拟文件纸）
    QRectF boundingRect = layout->blockBoundingRect(doc->begin());
    if (!boundingRect.isNull()) {
        // 左边界：第30像素处画一条红色装订线
        static const QColor bindingColor("#CC3333");
        painter.setPen(QPen(bindingColor, 1.5, Qt::SolidLine));
        painter.drawLine(QPointF(30.5, 0), QPointF(30.5, height()));
        painter.setPen(QPen(m_currentTheme.lineColor, m_currentTheme.lineHeight));
    } else {
        painter.setPen(QPen(m_currentTheme.lineColor, m_currentTheme.lineHeight));
    }

    // 遍历文档每个文本块（每个段落）
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        if (!block.isVisible()) continue;

        const QRectF blockRect = layout->blockBoundingRect(block);
        const qreal blockTop  = blockRect.top();

        // 如果整个块在可见区域之上，跳过
        if (blockRect.bottom() < exposedRect.top() - scrollY) continue;
        // 如果整个块在可见区域之下，停止
        if (blockTop > exposedRect.bottom() - scrollY) break;

        QTextLayout* tl = block.layout();
        if (!tl) continue;

        // 遍历块内的每一行
        const int lineCount = tl->lineCount();
        for (int i = 0; i < lineCount; ++i) {
            QTextLine line = tl->lineAt(i);
            qreal lineY = blockTop + line.rect().top();

            // 计算行底线在视口中的位置（加上滚动偏移）
            qreal lineBottomViewport = lineY + line.rect().height() - scrollY;

            // 画横线：底线贴齐文字底部
            painter.drawLine(
                QPointF(0.5,       lineBottomViewport),
                QPointF(width() - 1, lineBottomViewport)
            );
        }
    }
}

void LinedTextEdit::paintEvent(QPaintEvent* event)
{
    // 先用默认绘制把文字等内容画上去
    QTextEdit::paintEvent(event);

    // 再叠加信签纸横线
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing, false);
    drawLinedBackground(painter, viewport()->rect());
}

void LinedTextEdit::resizeEvent(QResizeEvent* event)
{
    QTextEdit::resizeEvent(event);
    // 窗口大小变化时重绘
    update();
}
