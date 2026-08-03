#include "load_options_codec.h"

namespace opentm::tm_ui {

namespace {

template <typename T>
struct field {
    const char* name;
    T load_options::* ptr;
};

constexpr field<bool> kBools[] = {
    {"reset_target",             &load_options::reset_target},
    {"clear_streams",            &load_options::clear_streams},
    {"enable_debug_module",      &load_options::enable_debug_module},
    {"disable_ppu_debug",        &load_options::disable_ppu_debug},
    {"disable_spu_debug",        &load_options::disable_spu_debug},
    {"wait_for_bdvd",            &load_options::wait_for_bdvd},
    {"use_elf_priority",         &load_options::use_elf_priority},
    {"use_elf_stack",            &load_options::use_elf_stack},
    {"enable_extra_options",     &load_options::enable_extra_options},
    {"lv2_exception_handler",    &load_options::lv2_exception_handler},
    {"remote_play",              &load_options::remote_play},
    {"gcm_debug",                &load_options::gcm_debug},
    {"load_libprof",             &load_options::load_libprof},
    {"core_dump",                &load_options::core_dump},
    {"remote_play_avc",          &load_options::remote_play_avc},
    {"smart_image_capture",      &load_options::smart_image_capture},
    {"memory_access_trap",       &load_options::memory_access_trap},
    {"patch_boot",               &load_options::patch_boot},
    {"rsx_profiling_tool",       &load_options::rsx_profiling_tool},
    {"high_memory_footprint",    &load_options::high_memory_footprint},
    {"gcm_capture_mode",         &load_options::gcm_capture_mode},
    {"paramsfo_mapping",         &load_options::paramsfo_mapping},
    {"paramsfo_use_elf_dir",     &load_options::paramsfo_use_elf_dir},
    {"rsx_hud",                  &load_options::rsx_hud},
    {"rsx_hud_start_on",         &load_options::rsx_hud_start_on},
    {"trig_disable_ppu_exc",     &load_options::trig_disable_ppu_exc},
    {"trig_disable_spu_exc",     &load_options::trig_disable_spu_exc},
    {"trig_disable_rsx_exc",     &load_options::trig_disable_rsx_exc},
    {"trig_disable_footswitch",  &load_options::trig_disable_footswitch},
    {"corefile_disable_memdump", &load_options::corefile_disable_memdump},
    {"exec_restart_after_dump",  &load_options::exec_restart_after_dump},
    {"exec_dump_fn_after_dump",  &load_options::exec_dump_fn_after_dump},
    {"bootable_msg_mapping",     &load_options::bootable_msg_mapping},
    {"bootable_msg_use_elf_dir", &load_options::bootable_msg_use_elf_dir},
};

constexpr field<QString> kStrings[] = {
    {"cmdline",          &load_options::cmdline},
    {"home_dir",         &load_options::home_dir},
    {"paramsfo_path",    &load_options::paramsfo_path},
    {"bootable_msg_dir", &load_options::bootable_msg_dir},
};

constexpr field<std::uint32_t> kU32[] = {
    {"priority",   &load_options::priority},
    {"stack_size", &load_options::stack_size},
};

} // namespace

QJsonObject load_options_to_json(const load_options& o) {
    QJsonObject j;
    for (const auto& f : kBools)   j[QLatin1String(f.name)] = o.*f.ptr;
    for (const auto& f : kStrings) j[QLatin1String(f.name)] = o.*f.ptr;

    for (const auto& f : kU32) {
        j[QLatin1String(f.name)] = QString::number(o.*f.ptr);
    }

    j["game_attribute"]     = QString::number(o.game_attribute);
    j["core_dump_location"] = QString::number(o.core_dump_location);
    return j;
}

load_options load_options_from_json(const QJsonObject& j) {
    load_options o;
    auto num = [&j](const char* key, quint64 fallback) -> quint64 {
        const auto v = j.value(QLatin1String(key));
        if (v.isUndefined() || v.isNull()) return fallback;
        return v.isString() ? v.toString().toULongLong(nullptr, 0) : static_cast<quint64>(v.toDouble());
    };

    for (const auto& f : kBools) {
        const auto v = j.value(QLatin1String(f.name));
        if (!v.isUndefined()) o.*f.ptr = v.toBool(o.*f.ptr);
    }
    for (const auto& f : kStrings) {
        const auto v = j.value(QLatin1String(f.name));
        if (!v.isUndefined()) o.*f.ptr = v.toString();
    }
    for (const auto& f : kU32) {
        o.*f.ptr = static_cast<std::uint32_t>(num(f.name, o.*f.ptr));
    }
    o.game_attribute     = static_cast<std::uint8_t>(num("game_attribute", o.game_attribute));
    o.core_dump_location = num("core_dump_location", o.core_dump_location);
    return o;
}

bool operator==(const load_options& a, const load_options& b) {
    for (const auto& f : kBools)   if (a.*f.ptr != b.*f.ptr) return false;
    for (const auto& f : kStrings) if (a.*f.ptr != b.*f.ptr) return false;
    for (const auto& f : kU32)     if (a.*f.ptr != b.*f.ptr) return false;
    return a.game_attribute     == b.game_attribute
        && a.core_dump_location == b.core_dump_location;
}

} // namespace opentm::tm_ui
