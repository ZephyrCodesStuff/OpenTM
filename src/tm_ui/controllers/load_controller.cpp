#include "load_controller.h"
#include <tm_session/session_api.h>
#include "../load_executable_dialog.h"

#include <QFileInfo>
#include <QWidget>

namespace opentm::tm_ui {

load_controller::load_controller(session_api* session, QWidget* dialog_parent, QObject* parent) : QObject(parent), session_(session), dialog_parent_(dialog_parent) {}

load_controller::~load_controller() = default;

void load_controller::set_active_target_defaults(const QString& app_home, const QString& home_dir)
{
    default_app_home_ = app_home;
    default_home_    = home_dir;
}

void load_controller::load_and_run() {
    load_executable_dialog dlg(default_app_home_, default_home_, dialog_parent_);
    if (dlg.exec() != QDialog::Accepted) return;
    const auto r = dlg.selection();
    if (r.path.isEmpty()) return;

    if (session_) {
        if (!r.file_serving_dir.isEmpty()) {
            session_->set_file_serving_dir(r.file_serving_dir);
        } else if (!r.is_device_path) {
            session_->set_file_serving_dir(QFileInfo(r.path).absolutePath());
        }
    }
    if (session_) {
        load_options opts = base_options_;
        opts.cmdline             = r.cmdline;
        opts.home_dir            = r.home_dir;
        opts.reset_target        = r.reset_target;
        opts.clear_streams       = r.clear_streams;
        opts.enable_debug_module = r.enable_debug_module;
        opts.disable_ppu_debug   = r.disable_ppu_debug;
        opts.disable_spu_debug   = r.disable_spu_debug;
        session_->load_executable(r.path, opts);
    }
}

void load_controller::on_load_ext_reply(std::uint32_t lv2_status) {
    QString meaning;
    switch (lv2_status) {
    case 0x00000000: meaning = tr("OK"); break;
    case 0x80010002: meaning = tr("LV2_EPERM (operation not permitted)"); break;
    case 0x80010006: meaning = tr("LV2_ENOENT (file not found)"); break;
    case 0x80010008: meaning = tr("LV2_EACCES (SELF auth / permission denied)"); break;
    case 0x80010009: meaning = tr("LV2_EFAULT"); break;
    case 0x8001000d: meaning = tr("LV2_EAGAIN (resource busy)"); break;
    case 0x8001001d: meaning = tr("LV2_EINVAL"); break;
    case 0x80028f90: meaning = tr("process already running - reset target or terminate the running process first"); break;
    default:         meaning = tr("LV2 status 0x%1").arg(lv2_status, 8, 16, QChar('0')); break;
    }
    emit log_message(tr("       >> LOAD_EXT reply: %1").arg(meaning));
    emit status_message(tr("Load: %1").arg(meaning));
}

} // namespace opentm::tm_ui
