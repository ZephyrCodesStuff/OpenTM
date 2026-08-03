#pragma once

#include <QString>
#include <QStringList>

namespace opentm::tm_ui {

void apply_saved_style();
void set_saved_style(const QString& name);
QString saved_style();
QStringList available_styles();

} // namespace opentm::tm_ui
