#pragma once

#include <QString>

namespace opentm::tm_ui {

bool size_units_are_binary();
void set_size_units_binary(bool on);

// last directories used by the file dialogs, so a second download does not start back at the home folder
QString last_download_dir();
void    set_last_download_dir(const QString& dir);
QString last_upload_dir();
void    set_last_upload_dir(const QString& dir);
QString last_save_dir();
void    set_last_save_dir(const QString& dir);

bool use_tray_supervisor();
void set_use_tray_supervisor(bool on);
bool auto_reconnect_enabled();
void set_auto_reconnect_enabled(bool on);
bool auto_arp_enabled();
void set_auto_arp_enabled(bool on);

QString console_font_family();
int     console_font_point_size();
void    set_console_font(const QString& family, int point_size);

} // namespace opentm::tm_ui
