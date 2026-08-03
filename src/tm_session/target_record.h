#pragma once

#include "load_options.h"

#include <tm_core/target_type.h>

#include <QString>

namespace opentm::tm_ui {

struct target_record {
    QString                       id;
    QString                       name;
    opentm::tm_core::target_type  type = opentm::tm_core::target_type::decr_tcp;
    QString                       host;
    quint16                       port = 8530;
    QString                       mac;
    QString                       file_server_dir;
    QString                       home_dir;
    opentm::tm_core::target_timeouts timeouts;
    load_options                     load;
    bool    force_case_sensitive = false;
    bool    env_var_expansion    = false;
    QString events_to_log =
        QStringLiteral("READ | WRITE | SEEK | TRUNCATE | CREATE");
    int     file_serving_log_size = 4096;
    int     file_trace_log_size   = 65536;
    int     console_cache_kb      = 1024;
    bool    image_capture_auto    = false;
    QString image_capture_dir;
    enum reset_mode_t { reset_debug_mode = 0, reset_ssm_mode = 1, reset_release_mode = 2, reset_advanced = 3 };
    int     reset_mode         = reset_debug_mode;
    quint64 reset_boot_value   = 0x10;
    quint64 reset_boot_mask    = 0x11;
    quint64 reset_system_value = 0x0;
    quint64 reset_system_mask  = 0x0;
    bool    display_reset_settings = true;
};

} // namespace opentm::tm_ui
