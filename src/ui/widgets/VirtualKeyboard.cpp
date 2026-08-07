#include "VirtualKeyboard.h"

#include <QFile>
#include <spdlog/spdlog.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QApplication>
#include <QStyle>

// ============================================================
//  98 键 (1800 紧凑布局) 键位定义
//  每行: { 显示文字, 宽度单位(1u≈48px), 发送键值(空=用显示文字) }
// ============================================================

struct KeyDef {
    QString label;
    float   width;      // 宽度单位 (1u = 标准键宽)
    QString value;      // 发出的键值，留空则使用 label
};

// 1 单位像素宽度
static constexpr int kUnitW = 48;
static constexpr int kKeyH  = 44;
static constexpr int kGap   = 4;

static const QVector<QVector<KeyDef>> kLayout = {
    // ── Row 0: F 键区 ──
    {
        {QStringLiteral("Esc"),  1.0f},
        {QStringLiteral("F1"),   1.0f},
        {QStringLiteral("F2"),   1.0f},
        {QStringLiteral("F3"),   1.0f},
        {QStringLiteral("F4"),   1.0f},
        {QStringLiteral("F5"),   1.0f},
        {QStringLiteral("F6"),   1.0f},
        {QStringLiteral("F7"),   1.0f},
        {QStringLiteral("F8"),   1.0f},
        {QStringLiteral("F9"),   1.0f},
        {QStringLiteral("F10"),  1.0f},
        {QStringLiteral("F11"),  1.0f},
        {QStringLiteral("F12"),  1.0f},
        {QStringLiteral("Del"),  1.0f, QStringLiteral("Delete")},
        {QStringLiteral("PrtSc"),1.0f, QStringLiteral("Print")},
    },
    // ── Row 1: 数字键 ──
    {
        {QStringLiteral("`"),  1.0f},
        {QStringLiteral("1"),  1.0f},
        {QStringLiteral("2"),  1.0f},
        {QStringLiteral("3"),  1.0f},
        {QStringLiteral("4"),  1.0f},
        {QStringLiteral("5"),  1.0f},
        {QStringLiteral("6"),  1.0f},
        {QStringLiteral("7"),  1.0f},
        {QStringLiteral("8"),  1.0f},
        {QStringLiteral("9"),  1.0f},
        {QStringLiteral("0"),  1.0f},
        {QStringLiteral("-"),  1.0f},
        {QStringLiteral("="),  1.0f},
        {QStringLiteral("←"),  2.0f, QStringLiteral("Backspace")},
    },
    // ── Row 2: QWERTY + Home ──
    {
        {QStringLiteral("Tab"),    1.50f},
        {QStringLiteral("Q"),      1.0f},
        {QStringLiteral("W"),      1.0f},
        {QStringLiteral("E"),      1.0f},
        {QStringLiteral("R"),      1.0f},
        {QStringLiteral("T"),      1.0f},
        {QStringLiteral("Y"),      1.0f},
        {QStringLiteral("U"),      1.0f},
        {QStringLiteral("I"),      1.0f},
        {QStringLiteral("O"),      1.0f},
        {QStringLiteral("P"),      1.0f},
        {QStringLiteral("["),      1.0f},
        {QStringLiteral("]"),      1.0f},
        {QStringLiteral("\\"),     1.0f},
        {QStringLiteral("Home"),   1.0f},
    },
    // ── Row 3: Home row + End ──
    {
        {QStringLiteral("Caps"),   1.75f, QStringLiteral("CapsLock")},
        {QStringLiteral("A"),      1.0f},
        {QStringLiteral("S"),      1.0f},
        {QStringLiteral("D"),      1.0f},
        {QStringLiteral("F"),      1.0f},
        {QStringLiteral("G"),      1.0f},
        {QStringLiteral("H"),      1.0f},
        {QStringLiteral("J"),      1.0f},
        {QStringLiteral("K"),      1.0f},
        {QStringLiteral("L"),      1.0f},
        {QStringLiteral(";"),      1.0f},
        {QStringLiteral("'"),      1.0f},
        {QStringLiteral("Enter"),  2.25f},
        {QStringLiteral("End"),    1.0f},
    },
    // ── Row 4: Shift row + PgUp ──
    {
        {QStringLiteral("Shift"),     2.25f, QStringLiteral("Shift")},
        {QStringLiteral("Z"),         1.0f},
        {QStringLiteral("X"),         1.0f},
        {QStringLiteral("C"),         1.0f},
        {QStringLiteral("V"),         1.0f},
        {QStringLiteral("B"),         1.0f},
        {QStringLiteral("N"),         1.0f},
        {QStringLiteral("M"),         1.0f},
        {QStringLiteral(","),         1.0f},
        {QStringLiteral("."),         1.0f},
        {QStringLiteral("/"),         1.0f},
        {QStringLiteral("Shift"),     1.75f, QStringLiteral("Shift")},
        {QStringLiteral("↑"),         1.0f,  QStringLiteral("Up")},
        {QStringLiteral("PgUp"),      1.0f,  QStringLiteral("PageUp")},
    },
    // ── Row 5: 底部控制键 + PgDn ──
    {
        {QStringLiteral("Ctrl"),      1.50f, QStringLiteral("Control")},
        {QStringLiteral("Win"),       1.25f, QStringLiteral("Meta")},
        {QStringLiteral("Alt"),       1.25f, QStringLiteral("Alt")},
        {QStringLiteral(""),          6.25f, QStringLiteral("Space")},
        {QStringLiteral("Alt"),       1.25f, QStringLiteral("Alt")},
        {QStringLiteral("Win"),       1.25f, QStringLiteral("Meta")},
        {QStringLiteral("Ctrl"),      1.50f, QStringLiteral("Control")},
        {QStringLiteral("←"),         1.0f,  QStringLiteral("Left")},
        {QStringLiteral("↓"),         1.0f,  QStringLiteral("Down")},
        {QStringLiteral("→"),         1.0f,  QStringLiteral("Right")},
        {QStringLiteral("PgDn"),      1.0f,  QStringLiteral("PageDown")},
    },
};

// ============================================================

VirtualKeyboard::VirtualKeyboard(QWidget *parent)
    : QWidget(parent) {
    setupUI();
    loadStyleSheet();
}

QPushButton *VirtualKeyboard::createKey(const QString &text,
                                         float widthUnits,
                                         const QString &keyValue) {
    auto *btn = new QPushButton(text, this);
    btn->setFixedHeight(kKeyH);
    btn->setFixedWidth(static_cast<int>(widthUnits * kUnitW + (widthUnits - 1.0f) * kGap));
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setCursor(Qt::PointingHandCursor);

    // 样式类：普通键 / 功能键
    const bool isSpecial = (widthUnits > 1.0f || text.isEmpty() || text.length() > 2);
    btn->setProperty("keyClass", isSpecial ? "special" : "alpha");

    // 空格键特殊处理
    if (keyValue == QStringLiteral("Space")) {
        btn->setProperty("keyClass", "space");
    }

    // 用 lambda 绑定键值，消除 sender() 反模式
    const QString emitValue = keyValue.isEmpty() ? text : keyValue;
    connect(btn, &QPushButton::clicked, this,
            [this, emitValue]() { emit keyPressed(emitValue); });

    return btn;
}

void VirtualKeyboard::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(kGap);
    mainLayout->setContentsMargins(kGap * 3, kGap * 2, kGap * 3, kGap * 2);

    for (const auto &rowDef : kLayout) {
        auto *rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(kGap);

        for (const auto &kd : rowDef) {
            auto *btn = createKey(kd.label,
                                  kd.width,
                                  kd.value);
            rowLayout->addWidget(btn);
        }

        // 行尾弹性空间（除最后一行外右端留空隙）
        rowLayout->addStretch();
        mainLayout->addLayout(rowLayout);
    }
}

void VirtualKeyboard::loadStyleSheet() {
    QFile f(QStringLiteral(":/keyboard_style.qss"));
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
        f.close();
    } else {
        spdlog::warn("VirtualKeyboard: failed to load :/keyboard_style.qss");
    }
}
