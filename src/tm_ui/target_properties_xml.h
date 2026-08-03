#pragma once

#include <tm_session/target_record.h>

#include <QByteArray>
#include <QString>

namespace opentm::tm_ui {

QByteArray target_properties_to_xml(const target_record& r);

bool target_properties_from_xml(const QByteArray& xml,target_record& out,QString* error = nullptr);

} // namespace opentm::tm_ui
