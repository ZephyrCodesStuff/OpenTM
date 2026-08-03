#pragma once

#include <tm_core/dbgp_codec.h>
#include <tm_core/deci3_codec.h>

#include <QByteArray>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace opentm::tm_ui {

inline std::vector<std::byte> to_bytes(const QByteArray& b) {
    const auto* p = reinterpret_cast<const std::byte*>(b.constData());
    return {p, p + b.size()};
}

inline QByteArray tsmp_inner(std::uint16_t inner_cmd, std::uint16_t sub_token, const QByteArray& extra = {}) {
    QByteArray b;
    const std::uint16_t inner_len = static_cast<std::uint16_t>(6 + 2 + extra.size());
    b.append(static_cast<char>(0x00));
    b.append(static_cast<char>(0x21));
    b.append(static_cast<char>(inner_len >> 8));
    b.append(static_cast<char>(inner_len & 0xff));
    b.append(static_cast<char>(inner_cmd >> 8));
    b.append(static_cast<char>(inner_cmd & 0xff));
    b.append(static_cast<char>(sub_token >> 8));
    b.append(static_cast<char>(sub_token & 0xff));
    if (!extra.isEmpty()) b.append(extra);
    return b;
}

inline opentm::tm_core::deci3_frame tsmp_frame_from_inner(const QByteArray& inner) {
    opentm::tm_core::deci3_frame f;
    f.direction = opentm::tm_core::deci3_direction::host_to_machine;
    f.category  = 0x0020;
    f.payload   = to_bytes(inner);
    return f;
}

inline void tsmp_retarget_for_dex(opentm::tm_core::deci3_frame& f) {
    f.direction = opentm::tm_core::deci3_direction::manager_to_target;
    f.session_b = 0x00ff0000u;
}

// cat=0x0200 dbgshl frame: 16-byte header + body + checksum trailer
inline opentm::tm_core::deci3_frame dbgshl_frame(std::uint32_t session_b, std::uint32_t ucmd, std::uint32_t seq, const QByteArray& body) {
    using namespace opentm::tm_core;
    dbgp::request r;
    r.cmd    = ucmd;
    r.req_id = seq;
    r.payload.assign(
        reinterpret_cast<const std::byte*>(body.constData()),
        reinterpret_cast<const std::byte*>(body.constData() + body.size()));
    deci3_frame f;
    f.direction = deci3_direction::host_to_target;
    f.category  = 0x0200;
    f.session_b = session_b;
    f.payload   = dbgp::encode_request(r);
    return f;
}

} // namespace opentm::tm_ui
