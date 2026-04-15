# PrisonSIS Design System — Government Blue Dark Theme

> 本文档为 PrisonSIS（监狱审讯笔录系统）的设计规范，定义了 UI 的视觉语言、色彩系统、排版规则、组件样式和布局原则。
>
> 设计参考：IBM Carbon Design System（政府/企业标准）+ HashiCorp（企业清洁美学）+ Linear.app（精确克制）+ Sentry（数据密集型仪表盘）
>
> 版本：1.0 | 状态：生产中

---

## 1. Visual Theme & Atmosphere

**视觉定位：** 政府政务系统 — 正式、权威、可信、数据密集、长时间使用舒适。

### 整体氛围
- **基调：** 深色沉稳（dark-first）+ 蓝色权威感
- **密度：** 高信息密度，数据密集型界面，适合专业司法人员长时间使用
- **情绪：** 严肃、精确、可信赖、政务正式感
- **交互哲学：** Linear 的精确克制 — 无多余装饰，每个像素都有功能

### 设计原则
1. **形式追随功能** — 不做装饰性设计，所有视觉元素都服务于信息传达
2. **层次分明** — 通过背景色层次建立信息优先级，不依赖阴影或高光
3. **色彩语义化** — 颜色传递状态含义，不是装饰
4. **对比度优先** — WCAG 2.1 AA 级可读性，主文字不低于 4.5:1 对比度
5. **克制一致** — 圆角、间距、字重都有系统规则，不随意添加

---

## 2. Color Palette & Roles

### 背景层次（Background Layers）

| 令牌 | 色值 | 用途 |
|------|------|------|
| `--bg-window` | `#0A0E14` | 窗口最底层背景 |
| `--bg-card` | `#0F1419` | 卡片、面板、对话框背景 |
| `--bg-nav` | `#161B22` | 导航栏、表格奇数行、分割区域 |
| `--bg-elevated` | `#1C2128` | 按钮悬停、表格偶数行、分隔线 |
| `--bg-overlay` | `#21262D` | 工具提示背景、下拉列表 |

### 主色调（Brand / Primary）

| 令牌 | 色值 | 用途 |
|------|------|------|
| `--primary` | `#0066CC` | 主操作按钮、聚焦边框、导航选中指示、链接 |
| `--primary-hover` | `#0052A3` | 主按钮悬停 |
| `--primary-pressed` | `#003D82` | 主按钮按下 |
| `--primary-subtle` | `#0066CC1A` | 主色背景（20% 透明度，用于选中行背景）|

### 语义色（Semantic Status Colors）

| 令牌 | 色值 | 用途 |
|------|------|------|
| `--success` | `#22C55E` | 成功、通过、已归档、正常状态 |
| `--success-bg` | `#22C55E1A` | 成功背景（20% 透明度） |
| `--danger` | `#E53E3E` | 危险、驳回、封存、删除、错误 |
| `--danger-bg` | `#E53E3E1A` | 危险背景（20% 透明度） |
| `--warning` | `#F59E0B` | 警告、待审批、待审核、进行中 |
| `--warning-bg` | `#F59E0B1A` | 警告背景（20% 透明度） |
| `--info` | `#3B82F6` | 信息提示、次要操作、次要链接 |
| `--info-bg` | `#3B82F61A` | 信息背景（20% 透明度） |
| `--purple` | `#8B5CF6` | 草稿、模板、信息标签（保留紫色系用于中性状态）|
| `--purple-bg` | `#8B5CF61A` | 紫色背景 |

### 文字色（Typography Colors）

| 令牌 | 色值 | 用途 |
|------|------|------|
| `--text-primary` | `#E6EDF3` | 正文主文字、标签 |
| `--text-secondary` | `#8B949E` | 次要文字、占位符提示 |
| `--text-disabled` | `#6E7681` | 禁用状态文字 |
| `--text-inverse` | `#FFFFFF` | 深色背景上的标题、强调文字 |
| `--text-on-color` | `#FFFFFF` | 按钮/标签上的白色文字 |
| `--text-placeholder` | `#484F58` | 输入框占位符、边框内文字 |

### 边框色（Border Colors）

| 令牌 | 色值 | 用途 |
|------|------|------|
| `--border-default` | `#21262D` | 默认边框、分割线 |
| `--border-hover` | `#30363D` | 悬停边框、分隔线 |
| `--border-focus` | `#0066CC` | 聚焦边框 |
| `--border-strong` | `#484F58` | 强调边框、分隔线 |

### 聚焦色（Interactive Focus）

| 令牌 | 色值 | 用途 |
|------|------|------|
| `--focus-ring` | `#0066CC` | 键盘聚焦环（通过 `outline` 实现）|

---

## 3. Typography Rules

### 字体栈

```css
font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Noto Sans CJK SC', 'Microsoft YaHei', sans-serif;
```

- **西欧字体首选：** Inter（Google Fonts，现代无衬线，精确克制）
- **macOS/iOS：** SF Pro
- **Windows：** Segoe UI
- **中文：** Noto Sans CJK SC（Google，开源 CJK）> Microsoft YaHei

### 字阶（Type Scale）

| 级别 | 字号 | 字重 | 字间距 | 用途 |
|------|------|------|--------|------|
| Display | 28px | 700 | -0.5px | 页面主标题、对话框标题 |
| Heading 1 | 22px | 600 | -0.3px | 区块标题、卡片标题 |
| Heading 2 | 18px | 600 | 0 | 副标题、表格列头 |
| Body Large | 14px | 400 | 0 | 正文大、重要信息 |
| Body | 13px | 400 | 0 | 正文、表单标签 |
| Label | 12px | 500 | 0.5px | 标签文字（uppercase）、按钮 |
| Caption | 11px | 400 | 0 | 辅助信息、次要说明 |

### 使用规则
- 所有文字使用 `--text-primary` 系列，不要硬编码颜色值
- 标题使用 `--text-inverse`（白色）或 `--text-primary`
- 标签使用 `--text-secondary`
- 按钮文字固定白色 `#FFFFFF`
- 代码/ID 字段使用等宽字体：`'JetBrains Mono', 'SF Mono', 'Consolas', monospace`

---

## 4. Component Stylings

### 4.1 Buttons（按钮）

#### 普通按钮（Secondary）

```css
/* 默认状态 */
QPushButton {
    background: #1C2128;
    color: #E6EDF3;
    border: 1px solid #21262D;
    border-radius: 4px;
    padding: 6px 14px;
    font-size: 13px;
    font-weight: 500;
}
/* 悬停 */
QPushButton:hover {
    background: #30363D;
    border-color: #30363D;
    color: #FFFFFF;
}
/* 按下 */
QPushButton:pressed {
    background: #161B22;
}
/* 禁用 */
QPushButton:disabled {
    background: #161B22;
    color: #6E7681;
    border-color: #21262D;
}
```

#### 主操作按钮（Primary / Brand）

```css
QPushButton#primaryBtn,
QPushButton[objectName="primaryBtn"] {
    background: #0066CC;
    color: #FFFFFF;
    border: none;
    border-radius: 4px;
    font-weight: 600;
    font-size: 13px;
    padding: 6px 14px;
}
QPushButton#primaryBtn:hover { background: #0052A3; }
QPushButton#primaryBtn:pressed { background: #003D82; }
```

#### 危险按钮（Danger）

```css
QPushButton#dangerBtn {
    background: #E53E3E;
    color: #FFFFFF;
    border: none;
    border-radius: 4px;
    font-weight: 600;
    font-size: 13px;
    padding: 6px 14px;
}
QPushButton#dangerBtn:hover { background: #C53030; }
```

#### 图标按钮

```css
QPushButton {
    /* 保持最小宽度，防止图标按钮变形 */
    min-width: 32px;
    min-height: 32px;
    padding: 4px 8px;
}
```

### 4.2 Inputs（输入框）

```css
QLineEdit, QTextEdit, QPlainTextEdit {
    background: #0F1419;
    color: #E6EDF3;
    border: 1px solid #21262D;
    border-radius: 4px;
    padding: 7px 10px;
    font-size: 13px;
    selection-background-color: #0066CC;
}
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
    border-color: #0066CC;
    background: #161B22;
}
QLineEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled {
    background: #0A0E14;
    color: #6E7681;
}
QLineEdit::placeholder { color: #484F58; }
```

### 4.3 ComboBox（下拉框）

```css
QComboBox {
    background: #0F1419;
    color: #E6EDF3;
    border: 1px solid #21262D;
    border-radius: 4px;
    padding: 7px 10px;
    font-size: 13px;
}
QComboBox:hover { border-color: #30363D; }
QComboBox:focus { border-color: #0066CC; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background: #161B22;
    color: #E6EDF3;
    border: 1px solid #21262D;
    border-radius: 4px;
    selection-background-color: #1C2128;
    padding: 4px;
}
```

### 4.4 Tables（表格）

```css
QTableWidget, QTreeWidget {
    background: #0A0E14;
    alternate-background-color: #0F1419;
    color: #E6EDF3;
    gridline-color: #21262D;
    border: 1px solid #21262D;
    border-radius: 6px;
    font-size: 13px;
}
QTableWidget::item, QTreeWidget::item {
    padding: 6px 8px;
    border-bottom: 1px solid #21262D;
}
QTableWidget::item:hover { background: #161B22; }
QTableWidget::item:selected { background: #0066CC33; color: #FFFFFF; }
QHeaderView::section {
    background: #161B22;
    color: #8B949E;
    font-weight: 600;
    font-size: 12px;
    padding: 8px 10px;
    border: none;
    border-bottom: 1px solid #21262D;
    border-right: 1px solid #21262D;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}
```

### 4.5 Navigation（导航栏）

```css
/* 侧边导航容器 */
QTreeWidget {
    background: #161B22;
    border: none;
    border-right: 1px solid #21262D;
    padding: 8px 0;
    outline: none;
}
/* 分类标题（如"核心业务"） */
QTreeWidget::item {
    color: #484F58;
    font-size: 11px;
    font-weight: 600;
    font-family: 'Inter', 'SF Pro', 'Segoe UI', sans-serif;
    padding: 12px 18px 4px;
    text-transform: uppercase;
    letter-spacing: 0.8px;
    min-height: 20px;
}
/* 菜单项 */
QTreeWidget::item {
    color: #8B949E;
    padding: 6px 18px;
    font-size: 13px;
    min-height: 28px;
}
QTreeWidget::item:hover {
    background: #1C2128;
    color: #E6EDF3;
}
/* 选中项：蓝色左边框 + 背景 */
QTreeWidget::item:selected {
    background: #0066CC1A;
    color: #FFFFFF;
    border-left: 2px solid #0066CC;
    padding-left: 16px;
}
```

### 4.6 Menu（菜单）

```css
QMenuBar {
    background: #0F1419;
    color: #E6EDF3;
    font-size: 13px;
    border-bottom: 1px solid #21262D;
    padding: 3px 0;
}
QMenuBar::item {
    padding: 5px 12px;
    border-radius: 4px;
}
QMenuBar::item:selected { background: #1C2128; color: #FFFFFF; }
QMenu {
    background: #161B22;
    border: 1px solid #21262D;
    border-radius: 6px;
    padding: 4px;
    font-size: 13px;
}
QMenu::item {
    padding: 6px 14px;
    border-radius: 4px;
    color: #E6EDF3;
}
QMenu::item:selected { background: #1C2128; color: #FFFFFF; }
QMenu::separator { height: 1px; background: #21262D; margin: 3px 6px; }
```

### 4.7 Status Indicators（状态指示器）

```css
/* 成功状态 */
QLabel#statusSuccess {
    color: #22C55E;
    background: #22C55E1A;
    border: 1px solid #22C55E33;
    border-radius: 4px;
    padding: 2px 8px;
    font-size: 12px;
    font-weight: 500;
}
/* 危险状态 */
QLabel#statusDanger {
    color: #E53E3E;
    background: #E53E3E1A;
    border: 1px solid #E53E3E33;
    border-radius: 4px;
    padding: 2px 8px;
    font-size: 12px;
    font-weight: 500;
}
/* 警告状态 */
QLabel#statusWarning {
    color: #F59E0B;
    background: #F59E0B1A;
    border: 1px solid #F59E0B33;
    border-radius: 4px;
    padding: 2px 8px;
    font-size: 12px;
    font-weight: 500;
}
/* 信息状态 */
QLabel#statusInfo {
    color: #3B82F6;
    background: #3B82F61A;
    border: 1px solid #3B82F633;
    border-radius: 4px;
    padding: 2px 8px;
    font-size: 12px;
    font-weight: 500;
}
```

### 4.8 GroupBox

```css
QGroupBox {
    font-weight: 600;
    font-size: 12px;
    color: #8B949E;
    border: 1px solid #21262D;
    border-radius: 6px;
    margin-top: 10px;
    padding: 8px 12px;
    background: transparent;
}
QGroupBox::title {
    color: #0066CC;
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 6px;
    font-weight: 600;
    font-size: 11.5px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}
```

### 4.9 ScrollBar（滚动条）

```css
QScrollBar:vertical {
    background: #0A0E14;
    width: 6px;
    border-radius: 3px;
    margin: 2px;
}
QScrollBar::handle:vertical {
    background: #21262D;
    border-radius: 3px;
    min-height: 30px;
}
QScrollBar::handle:vertical:hover { background: #30363D; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar:horizontal {
    background: #0A0E14;
    height: 6px;
    border-radius: 3px;
    margin: 2px;
}
QScrollBar::handle:horizontal {
    background: #21262D;
    border-radius: 3px;
    min-width: 30px;
}
QScrollBar::handle:horizontal:hover { background: #30363D; }
```

### 4.10 Tooltip / MessageBox

```css
QToolTip {
    background: #21262D;
    color: #E6EDF3;
    border: 1px solid #30363D;
    border-radius: 4px;
    padding: 5px 8px;
    font-size: 12px;
}
QMessageBox {
    background: #161B22;
}
QMessageBox QLabel { color: #E6EDF3; }
```

---

## 5. Layout Principles

### 间距系统（4px 基准网格）

```
$space-xs:  4px   — 紧凑元素内部
$space-sm:  8px   — 元素间距
$space-md:  12px  — 控件内边距
$space-lg:  16px  — 卡片内边距
$space-xl:  20px  — section 内间距
$space-2xl: 24px  — section 间距
$space-3xl: 32px  — 页面边距
```

### 主窗口布局

```
QMainWindow
├── QMenuBar（顶部菜单）
├── Central Widget（QHBoxLayout，margins=0, spacing=0）
│   ├── QTreeWidget（侧边导航，fixedWidth=210px）
│   ├── Separator Line（1px vertical line）
│   └── QStackedWidget（主内容区，stretch=1）
└── QStatusBar（底部状态栏）
```

### 卡片式布局原则
- 所有内容区块使用卡片（`background: #0F1419`，`border: 1px solid #21262D`，`border-radius: 6px`）
- 卡片内边距：`16px`（`space-lg`）
- 卡片间距：`16px`（`space-lg`）
- section 标题上方留 `24px` 间距

### 对话框布局
- 固定宽度对话框：顶部 4px 政府蓝渐变条
- 对话框容器：`border-radius: 12px`，`background: #0F1419`
- 内层卡片：`border-radius: 8px`，`padding: 24px 28px`

---

## 6. Depth & Elevation

### 阴影系统（HashiCorp/Linear 风格，轻微 elevation）

```css
/* 卡片阴影 */
#card {
    box-shadow: none; /* 默认无阴影，依靠背景色层次 */
    /* 如需阴影：0 1px 3px rgba(0,0,0,0.3) */
}
/* 对话框阴影 */
QDialog {
    box-shadow: 0 4px 24px rgba(0,0,0,0.6);
}
/* 悬停提升 */
QPushButton:hover {
    box-shadow: 0 2px 8px rgba(0,0,0,0.3);
}
/* 聚焦环 */
*:focus {
    outline: 2px solid #0066CC;
    outline-offset: 2px;
}
```

### 层级设计
- **Layer 0**（最底层）：窗口背景 `#0A0E14`
- **Layer 1**（内容层）：卡片/面板 `#0F1419`
- **Layer 2**（交互层）：导航栏、表格行交替 `#161B22`
- **Layer 3**（悬停层）：按钮悬停、聚焦状态 `#1C2128`
- **Layer 4**（最顶层）：下拉列表、工具提示 `#21262D`

---

## 7. Do's and Don'ts

### ✅ 正确做法

1. **使用语义色** — 状态色必须与其含义对应（绿色=成功，红色=危险，黄色=警告）
2. **聚焦边框统一蓝色** — 所有输入框聚焦状态统一使用 `#0066CC`
3. **按钮状态完整** — 每个按钮必须定义 default / hover / pressed / disabled 四种状态
4. **文字对比度** — 主文字不低于 `#E6EDF3`（浅于 `#CACACA`）
5. **等宽字体用于 ID/代码** — 罪犯编号、案件编号使用等宽字体
6. **状态标签带背景** — 状态文字使用带透明背景色的标签（`background: 20% 透明度色），不要仅用文字颜色

### ❌ 错误做法

1. **不要用颜色单独表示状态** — 红色/绿色不能单独使用（色盲用户），必须配合图标或标签
2. **不要硬编码 `#5E6AD2`** — Linear 紫色是旧品牌色，所有 UI 必须使用 `#0066CC`
3. **不要混用色值** — 同一元素的状态色必须来自同一语义色系
4. **不要随意增加圆角** — 圆角大小按组件类型固定，不要一个地方用 4px 另一个用 8px
5. **不要用 `border-radius: 0`** — 所有交互元素至少 4px 圆角，保持现代感
6. **不要在表格中使用过多种颜色** — 表格背景仅用交替行色，信息通过文字和状态标签传达

---

## 8. Responsive Behavior（Qt Desktop）

### 窗口最小尺寸
```cpp
setMinimumSize(1280, 720); // 主窗口
```

### 表格列宽策略
- 关键信息列（如姓名、编号）：固定宽度或最小宽度
- 可变内容列：设置 `stretch=1` 或自适应
- 总宽度不足时：表格水平滚动，不压缩关键列

### 导航栏
- 固定宽度 210px（`setFixedWidth(210)`）
- 不随窗口缩放

### 分页设计
- 列表页面每页 20 条记录（硬编码）
- 大数据集始终使用分页，不做虚拟滚动

### 触摸友好
- 按钮最小点击区域：32px × 32px
- 触摸设备上加大 padding（`padding: 8px 16px`）

---

## 9. Agent Prompt Guide

### 快速参考：政府蓝暗色主题 QSS 片段

```css
/* 基础设置 */
QWidget {
    background: #0A0E14;
    color: #E6EDF3;
    font-family: 'Inter', 'SF Pro', 'Segoe UI', 'Noto Sans CJK SC', 'Microsoft YaHei', sans-serif;
}

/* 主操作按钮 */
QPushButton#primaryBtn {
    background: #0066CC; color: #FFFFFF; border: none;
    border-radius: 4px; padding: 6px 14px; font-weight: 600;
}

/* 聚焦边框 */
QLineEdit:focus, QComboBox:focus {
    border-color: #0066CC; background: #161B22;
}

/* 选中行 */
QTableWidget::item:selected { background: #0066CC33; color: #FFFFFF; }

/* 导航选中 */
QTreeWidget::item:selected {
    background: #0066CC1A; color: #FFFFFF;
    border-left: 2px solid #0066CC;
}

/* 状态色 */
--success: #22C55E; --danger: #E53E3E;
--warning: #F59E0B; --info: #3B82F6;
```

### 添加新组件时的 AI 提示模板

```
"为 PrisonSIS 添加一个 [组件名] widget，使用政府蓝暗色主题设计系统。
主色：#0066CC，背景：#0A0E14，卡片：#0F1419，文字：#E6EDF3。
按钮状态：default/hover/pressed/disabled，聚焦：#0066CC。
语义色：成功#22C55E，危险#E53E3E，警告#F59E0B，信息#3B82F6。
参考 DESIGN_SYSTEM.md 第 4 节组件样式。"
```

---

*文档版本：1.0 | 基于 VoltAgent/awesome-design-md 参考库 | 最后更新：2026-04-15*
