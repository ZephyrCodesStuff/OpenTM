#pragma once

#include "wol.h"

#include <QString>

#include <optional>

namespace opentm::tm_core {
    
std::optional<mac_address> arp_lookup(const QString& ipv4);

} // namespace opentm::tm_core
