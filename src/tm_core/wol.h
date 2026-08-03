#pragma once

#include <QtGlobal>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

class QHostAddress;

namespace opentm::tm_core {

using mac_address = std::array<std::uint8_t, 6>;

std::optional<mac_address> parse_mac(std::string_view s);
std::string format_mac(const mac_address& mac);
std::array<std::uint8_t, 102> build_magic_packet(const mac_address& mac);

bool send_wol(const mac_address& mac, const QHostAddress& broadcast, std::uint16_t port = 1000);
bool send_wol(const mac_address& mac, std::uint16_t port = 1000);

} // namespace opentm::tm_core
