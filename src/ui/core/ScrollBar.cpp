#include "ScrollBar.h"

#include <QFile>
#include <spdlog/spdlog.h>

static QString s_scrollbarStyleSheet;

static const QString &scrollbarStyleSheet() {
    if (s_scrollbarStyleSheet.isEmpty()) {
        QFile f(QStringLiteral(":/scrollbar_style.qss"));
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            s_scrollbarStyleSheet = QString::fromUtf8(f.readAll());
            f.close();
        } else {
            spdlog::warn("ScrollBarStyler: failed to load :/scrollbar_style.qss");
        }
    }
    return s_scrollbarStyleSheet;
}

void ScrollBarStyler::applyTo(QAbstractScrollArea *area) {
    if (!area) return;

    const QString currentStyle = area->styleSheet();
    const QString combined = currentStyle + scrollbarStyleSheet();
    area->setStyleSheet(combined);
}
