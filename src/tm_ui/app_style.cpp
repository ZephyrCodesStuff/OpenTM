#include "app_style.h"

#include <QApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>

namespace opentm::tm_ui {

namespace {
constexpr auto kKey = "ui_style";
}

QString saved_style() { return QSettings().value(QLatin1String(kKey)).toString(); }

QStringList available_styles() { return QStyleFactory::keys(); }

void apply_saved_style() {
    const auto name = saved_style();
    if (name.isEmpty()) return;
    if (auto* s = QStyleFactory::create(name)) QApplication::setStyle(s);
}

void set_saved_style(const QString& name) {
    if (auto* s = QStyleFactory::create(name)) {
        QApplication::setStyle(s);
        QSettings().setValue(QLatin1String(kKey), name);
    }
}

} // namespace opentm::tm_ui
