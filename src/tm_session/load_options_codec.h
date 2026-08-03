#pragma once

#include "load_options.h"

#include <QJsonObject>

namespace opentm::tm_ui {

QJsonObject   load_options_to_json(const load_options& o);
load_options  load_options_from_json(const QJsonObject& j);

bool operator==(const load_options& a, const load_options& b);
inline bool operator!=(const load_options& a, const load_options& b) {
    return !(a == b);
}

} // namespace opentm::tm_ui
