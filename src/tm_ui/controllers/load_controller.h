#pragma once

#include <tm_session/load_options.h>

#include <QObject>
#include <QString>

#include <cstdint>

class QWidget;

namespace opentm::tm_ui {

class session_api;
struct target_record;

class load_controller : public QObject {
    Q_OBJECT
public:
    load_controller(session_api* session, QWidget* dialog_parent, QObject* parent = nullptr);
    ~load_controller() override;

    void set_session(session_api* s) { session_ = s; }
    void set_active_target_defaults(const QString& app_home, const QString& home_dir);
    void set_base_load_options(const load_options& o) { base_options_ = o; }

public slots:
    void load_and_run();
    void on_load_ext_reply(std::uint32_t lv2_status);

signals:
    void log_message(QString line);
    void status_message(QString text);

private:
    load_options      base_options_;
    session_api*      session_       = nullptr;
    QWidget*          dialog_parent_ = nullptr;
    QString           default_app_home_;
    QString           default_home_;
};

} // namespace opentm::tm_ui
