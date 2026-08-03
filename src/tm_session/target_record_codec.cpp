#include "target_record_codec.h"

#include "load_options_codec.h"

#include <tm_core/target_type.h>

namespace opentm::tm_ui {

namespace {

using opentm::tm_core::target_type_string;
using opentm::tm_core::target_type_from_string;
using timeouts_t = opentm::tm_core::target_timeouts;

template <typename T, typename Owner>
struct field {
    const char* name;
    T Owner::* ptr;
};

constexpr field<QString, target_record> kStrings[] = {
    {"id",              &target_record::id},
    {"name",            &target_record::name},
    {"host",            &target_record::host},
    {"mac",             &target_record::mac},
    {"serve_dir",       &target_record::file_server_dir},
    {"home_dir",        &target_record::home_dir},
    {"events_to_log",   &target_record::events_to_log},
    {"image_capture_dir", &target_record::image_capture_dir},
};

constexpr field<bool, target_record> kBools[] = {
    {"force_case_sensitive",   &target_record::force_case_sensitive},
    {"env_var_expansion",      &target_record::env_var_expansion},
    {"image_capture_auto",     &target_record::image_capture_auto},
    {"display_reset_settings", &target_record::display_reset_settings},
};

constexpr field<int, target_record> kInts[] = {
    {"file_serving_log_size", &target_record::file_serving_log_size},
    {"file_trace_log_size",   &target_record::file_trace_log_size},
    {"console_cache_kb",      &target_record::console_cache_kb},
    {"reset_mode",            &target_record::reset_mode},
};

constexpr field<quint64, target_record> kU64[] = {
    {"reset_boot_value",   &target_record::reset_boot_value},
    {"reset_boot_mask",    &target_record::reset_boot_mask},
    {"reset_system_value", &target_record::reset_system_value},
    {"reset_system_mask",  &target_record::reset_system_mask},
};

constexpr field<int, timeouts_t> kTimeouts[] = {
    {"default",   &timeouts_t::default_ms},
    {"reset",     &timeouts_t::reset_ms},
    {"connect",   &timeouts_t::connect_ms},
    {"load",      &timeouts_t::load_ms},
    {"status",    &timeouts_t::status_ms},
    {"reconnect", &timeouts_t::reconnect_ms},
    {"game_port", &timeouts_t::game_port_ms},
    {"game_exit", &timeouts_t::game_exit_ms},
};

} // namespace

QJsonObject target_record_to_json(const target_record& r) {
    QJsonObject j;
    for (const auto& f : kStrings) j[QLatin1String(f.name)] = r.*f.ptr;
    for (const auto& f : kBools)   j[QLatin1String(f.name)] = r.*f.ptr;
    for (const auto& f : kInts)    j[QLatin1String(f.name)] = r.*f.ptr;
    for (const auto& f : kU64) {
        j[QLatin1String(f.name)] = QString::number(r.*f.ptr);
    }

    const auto sv = target_type_string(r.type);
    j["type"] = QString::fromLatin1(sv.data(), static_cast<int>(sv.size()));
    j["port"] = r.port;

    QJsonObject to;
    for (const auto& f : kTimeouts) to[QLatin1String(f.name)] = r.timeouts.*f.ptr;
    j["timeouts"] = to;

    j["load"] = load_options_to_json(r.load);
    return j;
}

target_record target_record_from_json(const QJsonObject& j) {
    target_record r;
    for (const auto& f : kStrings) {
        const auto v = j.value(QLatin1String(f.name));
        if (!v.isUndefined()) r.*f.ptr = v.toString();
    }
    for (const auto& f : kBools) {
        const auto v = j.value(QLatin1String(f.name));
        if (!v.isUndefined()) r.*f.ptr = v.toBool(r.*f.ptr);
    }
    for (const auto& f : kInts) {
        const auto v = j.value(QLatin1String(f.name));
        if (!v.isUndefined()) r.*f.ptr = v.toInt(r.*f.ptr);
    }
    for (const auto& f : kU64) {
        const auto v = j.value(QLatin1String(f.name));
        if (v.isUndefined() || v.isNull()) continue;
        r.*f.ptr = v.isString() ? v.toString().toULongLong(nullptr, 0) : static_cast<quint64>(v.toDouble());
    }

    if (const auto t = target_type_from_string(j.value("type").toString().toStdString())) {
        r.type = *t;
    }
    if (j.contains("port")) r.port = static_cast<quint16>(j.value("port").toInt());

    const auto to = j.value("timeouts").toObject();
    for (const auto& f : kTimeouts) {
        const auto v = to.value(QLatin1String(f.name));
        if (!v.isUndefined()) r.timeouts.*f.ptr = v.toInt(r.timeouts.*f.ptr);
    }

    if (j.contains("load")) r.load = load_options_from_json(j.value("load").toObject());
    return r;
}

} // namespace opentm::tm_ui
