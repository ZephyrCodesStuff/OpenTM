#include "app_prefs.h"

#include <QSettings>
#include <QStandardPaths>

namespace opentm::tm_ui {

namespace {
constexpr auto kUnitsKey  = "ui/size_units_binary";
constexpr auto kFontKey   = "ui/console_font_family";
constexpr auto kFontSzKey = "ui/console_font_size";
} // namespace

bool size_units_are_binary() {
    return QSettings().value(QLatin1String(kUnitsKey), true).toBool();
}

void set_size_units_binary(bool on) {
    QSettings().setValue(QLatin1String(kUnitsKey), on);
}

namespace {
QString dir_pref(const char* key) {
    const auto v = QSettings().value(QLatin1String(key)).toString();
    return v.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) : v;
}
void set_dir_pref(const char* key, const QString& dir) {
    if (!dir.isEmpty()) QSettings().setValue(QLatin1String(key), dir);
}
} // namespace

QString last_download_dir()                    { return dir_pref("ui/last_download_dir"); }
void    set_last_download_dir(const QString& d) { set_dir_pref("ui/last_download_dir", d); }
QString last_upload_dir()                      { return dir_pref("ui/last_upload_dir"); }
void    set_last_upload_dir(const QString& d)   { set_dir_pref("ui/last_upload_dir", d); }
QString last_save_dir()                        { return dir_pref("ui/last_save_dir"); }
void    set_last_save_dir(const QString& d)     { set_dir_pref("ui/last_save_dir", d); }

namespace {
bool flag(const char* key, bool dflt) {
    return QSettings().value(QLatin1String(key), dflt).toBool();
}
void set_flag(const char* key, bool on) { QSettings().setValue(QLatin1String(key), on); }
} // namespace

bool use_tray_supervisor()             { return flag("ui/use_tray_supervisor", true); }
void set_use_tray_supervisor(bool on)  { set_flag("ui/use_tray_supervisor", on); }
bool auto_reconnect_enabled()          { return flag("ui/auto_reconnect", true); }
void set_auto_reconnect_enabled(bool on) { set_flag("ui/auto_reconnect", on); }
bool auto_arp_enabled()                { return flag("ui/auto_arp", true); }
void set_auto_arp_enabled(bool on)     { set_flag("ui/auto_arp", on); }

QString console_font_family() {
    return QSettings().value(QLatin1String(kFontKey), QStringLiteral("Consolas")).toString();
}

int console_font_point_size() {
    return QSettings().value(QLatin1String(kFontSzKey), 10).toInt();
}

void set_console_font(const QString& family, int point_size) {
    QSettings s;
    s.setValue(QLatin1String(kFontKey), family);
    s.setValue(QLatin1String(kFontSzKey), point_size);
}

} // namespace opentm::tm_ui
