#include "deci3_codec.h"

#include "be_io.h"

#include <cstring>

namespace opentm::tm_core {

deci3_family family_of(deci3_direction d) noexcept {
    switch (d) {
        case deci3_direction::host_to_machine:
        case deci3_direction::machine_to_host:
            return deci3_family::netmp;
        case deci3_direction::host_to_target:
        case deci3_direction::target_to_host:
            return deci3_family::dfmp;
        case deci3_direction::manager_to_target:
        case deci3_direction::target_to_manager:
            return deci3_family::netmp_cfw;
    }
    return deci3_family::unknown;
}

std::string_view direction_name(deci3_direction d) noexcept {
    switch (d) {
        case deci3_direction::host_to_machine:   return "HM";
        case deci3_direction::machine_to_host:   return "MH";
        case deci3_direction::host_to_target:    return "HT";
        case deci3_direction::target_to_host:    return "TH";
        case deci3_direction::manager_to_target: return "MT";
        case deci3_direction::target_to_manager: return "TM";
    }
    return "??";
}

bool is_host_originated(deci3_direction d) noexcept {
    return d == deci3_direction::host_to_machine || d == deci3_direction::host_to_target || d == deci3_direction::manager_to_target;
}

std::size_t encode_frame(const deci3_frame& frame, std::vector<std::byte>& out) {
    const std::size_t total = frame.wire_size();
    const std::size_t start = out.size();
    out.resize(start + total);
    std::byte* p = out.data() + start;

    write_be_u16(p + 0,  deci3_magic);
    write_be_u32(p + 2,  frame.session_a);
    write_be_u16(p + 6,  static_cast<std::uint16_t>(total));
    write_be_u16(p + 8,  static_cast<std::uint16_t>(frame.direction));
    write_be_u32(p + 10, frame.session_b);
    write_be_u16(p + 14, frame.category);
    if (!frame.payload.empty()) {
        std::memcpy(p + deci3_header_size, frame.payload.data(), frame.payload.size());
    }
    return total;
}

decode_result decode_frame(std::span<const std::byte> buffer) noexcept {
    decode_result r{};
    if (buffer.size() < deci3_header_size) {
        r.error = decode_error::short_buffer;
        return r;
    }
    const std::byte* p = buffer.data();
    if (read_be_u16(p) != deci3_magic) {
        r.error = decode_error::bad_magic;
        return r;
    }
    const std::uint16_t length = read_be_u16(p + 6);
    if (length < deci3_header_size || length > buffer.size()) {
        r.error = decode_error::bad_length;
        return r;
    }
    deci3_frame f;
    f.session_a = read_be_u32(p + 2);
    f.direction = static_cast<deci3_direction>(read_be_u16(p + 8));
    f.session_b = read_be_u32(p + 10);
    f.category  = read_be_u16(p + 14);
    const std::size_t payload_size = length - deci3_header_size;
    f.payload.assign(p + deci3_header_size, p + deci3_header_size + payload_size);
    r.frame = std::move(f);
    r.consumed = length;
    return r;
}

std::vector<deci3_frame> decode_all(
    std::span<const std::byte> buffer,
    std::optional<decode_error>* error_out,
    std::size_t* error_offset_out) noexcept
{
    std::vector<deci3_frame> frames;
    std::size_t off = 0;
    while (off < buffer.size()) {
        auto r = decode_frame(buffer.subspan(off));
        if (r.frame) {
            frames.push_back(std::move(*r.frame));
            off += r.consumed;
        } else {
            if (error_out) *error_out = r.error;
            if (error_offset_out) *error_offset_out = off;
            return frames;
        }
    }
    if (error_out) *error_out = std::nullopt;
    if (error_offset_out) *error_offset_out = off;
    return frames;
}

} // namespace opentm::tm_core
