#pragma once

#include "target_record.h"

#include <QJsonObject>

namespace opentm::tm_ui {

QJsonObject   target_record_to_json(const target_record& r);
target_record target_record_from_json(const QJsonObject& j);

} // namespace opentm::tm_ui
