#include "drfp_codec.h"

namespace opentm::tm_core {

const char* drfp_code_name(drfp_code c) noexcept {
    switch (c) {
    case drfp_code::init:            return "INIT";
    case drfp_code::init_reply:      return "INIT_REPLY";
    case drfp_code::open:            return "OPEN";
    case drfp_code::open_reply:      return "OPEN_REPLY";
    case drfp_code::close:           return "CLOSE";
    case drfp_code::close_reply:     return "CLOSE_REPLY";
    case drfp_code::read:            return "READ";
    case drfp_code::read_reply:      return "READ_REPLY";
    case drfp_code::write:           return "WRITE";
    case drfp_code::write_reply:     return "WRITE_REPLY";
    case drfp_code::seek:            return "SEEK";
    case drfp_code::seek_reply:      return "SEEK_REPLY";
    case drfp_code::fstat:           return "FSTAT";
    case drfp_code::fstat_reply:     return "FSTAT_REPLY";
    case drfp_code::stat:            return "STAT";
    case drfp_code::stat_reply:      return "STAT_REPLY";
    case drfp_code::mkdir:           return "MKDIR";
    case drfp_code::mkdir_reply:     return "MKDIR_REPLY";
    case drfp_code::rmdir:           return "RMDIR";
    case drfp_code::rmdir_reply:     return "RMDIR_REPLY";
    case drfp_code::dopen:           return "DOPEN";
    case drfp_code::dopen_reply:     return "DOPEN_REPLY";
    case drfp_code::dclose:          return "DCLOSE";
    case drfp_code::dclose_reply:    return "DCLOSE_REPLY";
    case drfp_code::dread:           return "DREAD";
    case drfp_code::dread_reply:     return "DREAD_REPLY";
    case drfp_code::ftruncate:       return "FTRUNCATE";
    case drfp_code::ftruncate_reply: return "FTRUNCATE_REPLY";
    case drfp_code::truncate:        return "TRUNCATE";
    case drfp_code::truncate_reply:  return "TRUNCATE_REPLY";
    case drfp_code::rename:          return "RENAME";
    case drfp_code::rename_reply:    return "RENAME_REPLY";
    case drfp_code::unlink:          return "UNLINK";
    case drfp_code::unlink_reply:    return "UNLINK_REPLY";
    }
    return "UNKNOWN";
}

std::optional<drfp_frame> parse_drfp(const QByteArray& payload) {
    if (payload.size() < 8) return std::nullopt;
    drfp_frame f;
    const std::uint32_t raw_code = read_be_u32(payload, 0);
    if (raw_code > 33) return std::nullopt;
    f.code    = static_cast<drfp_code>(raw_code);
    f.seq     = read_be_u32(payload, 4);
    f.payload = payload.mid(8);
    return f;
}

QByteArray build_drfp(drfp_code code, std::uint32_t seq, const QByteArray& payload) {
    QByteArray out;
    out.reserve(8 + payload.size());
    append_be_u32(out, static_cast<std::uint32_t>(code));
    append_be_u32(out, seq);
    out.append(payload);
    return out;
}

QByteArray build_init_reply(std::uint32_t seq) {
    QByteArray body;
    body.reserve(8);
    append_be_u32(body, 0);
    append_be_u32(body, 15);
    return build_drfp(drfp_code::init_reply, seq, body);
}

QByteArray build_open_reply(std::uint32_t seq, std::int32_t result, std::uint32_t fd) {
    QByteArray body;
    body.reserve(8);
    append_be_u32(body, static_cast<std::uint32_t>(result));
    append_be_u32(body, fd);
    return build_drfp(drfp_code::open_reply, seq, body);
}

QByteArray build_write_reply(std::uint32_t seq, std::int32_t result, std::uint32_t nbytes) {
    QByteArray body;
    body.reserve(8);
    append_be_u32(body, static_cast<std::uint32_t>(result));
    append_be_u32(body, result == 0 ? nbytes : 0u);
    return build_drfp(drfp_code::write_reply, seq, body);
}

QByteArray build_dopen_reply(std::uint32_t seq, std::int32_t result, std::uint32_t fd) {
    QByteArray body;
    body.reserve(8);
    append_be_u32(body, static_cast<std::uint32_t>(result));
    append_be_u32(body, result == 0 ? fd : 0xffffffffu);
    return build_drfp(drfp_code::dopen_reply, seq, body);
}

QByteArray build_dirent(std::uint8_t d_type, const QByteArray& name) {
    // struct { u8 d_type; u8 d_namesize; signed char d_name[256]; }
    constexpr int kNameSlot = 256;
    QByteArray trimmed = name.left(kNameSlot - 1);
    QByteArray out;
    out.reserve(2 + kNameSlot);
    out.append(static_cast<char>(d_type));
    out.append(static_cast<char>(trimmed.size() & 0xff));
    out.append(trimmed);
    out.append(QByteArray(kNameSlot - trimmed.size(), '\0'));
    return out;
}

QByteArray build_dread_reply(std::uint32_t seq, std::int32_t result, const QByteArray& entry) {
    QByteArray body;
    body.reserve(8 + entry.size());
    append_be_u32(body, static_cast<std::uint32_t>(result));
    if (result != 0) {
        append_be_u32(body, 0);
        return build_drfp(drfp_code::dread_reply, seq, body);
    }
    append_be_u32(body, static_cast<std::uint32_t>(entry.size()));
    body.append(entry);
    return build_drfp(drfp_code::dread_reply, seq, body);
}

QByteArray build_result_reply(drfp_code reply_code, std::uint32_t seq, std::int32_t result) {
    QByteArray body;
    append_be_u32(body, static_cast<std::uint32_t>(result));
    return build_drfp(reply_code, seq, body);
}

QByteArray build_read_reply(std::uint32_t seq, std::int32_t result, const QByteArray& data) {
    QByteArray body;
    body.reserve(8 + data.size());
    append_be_u32(body, static_cast<std::uint32_t>(result));
    append_be_u32(body, static_cast<std::uint32_t>(data.size()));
    body.append(data);
    return build_drfp(drfp_code::read_reply, seq, body);
}

QByteArray build_stat_reply(drfp_code reply_code, std::uint32_t seq, std::int32_t result, const drfp_stat& st) {
    QByteArray body;
    body.reserve(4 + 44);
    append_be_u32(body, static_cast<std::uint32_t>(result));
    append_be_u32(body, st.mode);
    append_be_u32(body, st.unknown_a);
    append_be_u32(body, st.unknown_b);
    append_be_u64(body, st.mtime);
    append_be_u64(body, st.atime);
    append_be_u64(body, st.ctime);
    append_be_u32(body, st.unknown_c);
    append_be_u32(body, st.size);
    return build_drfp(reply_code, seq, body);
}

} // namespace opentm::tm_core
