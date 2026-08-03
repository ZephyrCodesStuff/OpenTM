#pragma once

#include <tm_session/session_api.h>
#include <tm_session/session_factory.h>
#include <tm_session/target_session.h>

#include <tm_core/deci3_codec.h>
#include <tm_core/target_type.h>
#include <tm_core/tcp_connection.h>

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPersistentModelIndex>
#include <QVector>

#include <cstdint>

class QAction;
class QActionGroup;
class QDockWidget;
class QLabel;
class QMenu;
class QProgressDialog;
class QStackedWidget;
class QTabWidget;
class QToolBar;
class QPoint;

namespace opentm::tm_ui {

class console_panel;
class cp_panel;
class file_explorer_controller;
class file_explorer_panel;
class frame_dispatcher;
class host_file_server;
class kernel_explorer_controller;
class kernel_explorer_panel;
class load_controller;
class session_controller;
class target_actions;
class target_panel;
class wire_log_panel;
struct target_record;

class main_window : public QMainWindow {
    Q_OBJECT
public:
    explicit main_window(const session_backend& backend = {}, QWidget* parent = nullptr);
    ~main_window() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_connect_target();
    void on_disconnect_target();
    void on_install_package();

    void on_target_selection_changed();
    void on_target_context_menu(QPoint global_pos, bool has_target);

    void on_connection_state(opentm::tm_core::tcp_connection::state s);
    void on_greeting_received(QByteArray bytes);
    void on_connection_error(QString msg);

    void on_session_ready_changed(std::uint16_t token, std::uint16_t sub_token);
    void on_session_invalidated();

    void on_probe_handshake_acked();
    void kick_next_probe();

private:
    void build_menus();
    void register_shortcuts();
    void open_preferences();
    void set_cp_tab_visible(bool on);
    void save_text_to_file(const QString& text, const QString& suggested_name);
    void name_action(const QString& key, QAction* a, const QKeySequence& def);
    void build_toolbar();
    void build_target_dock();
    void build_central();
    void build_wire_log();
    void build_console();
    void build_status_indicator();
    void update_action_states();
    void update_state_indicator(opentm::tm_core::tcp_connection::state s);
    void set_status_for_current(const QString& s);
    void stub_status(const QString& msg);
    void log_wire(const QString& line);

    struct target_slot {
        session_api*           session = nullptr;
        QPersistentModelIndex  row;
        file_explorer_panel*   files   = nullptr;
        kernel_explorer_panel* kernel  = nullptr;
        console_panel*         console = nullptr;
        wire_log_panel*        wire    = nullptr;
        cp_panel*              cp      = nullptr;
        QString                pending_transfer;
        QString                refresh_after_transfer;
    };

    target_slot* slot_for(const target_record& r, const QPersistentModelIndex& idx);

    void run_from_explorer(target_slot* slot, const QString& kit_path);
    void activate(target_slot* slot);
    void wire_active_session();
    void wire_slot_panels(target_slot* slot);
    void close_slot(const QString& id);
    void on_debug_agent_ready(target_slot* slot);

    wire_log_panel*        active_wire() const;
    console_panel*         active_console() const;
    file_explorer_panel*   active_files() const;
    kernel_explorer_panel* active_kernel() const;

    session_backend                  backend_;
    QHash<QString, target_slot*>     slots_;
    target_slot*                     active_ = nullptr;
    QList<QMetaObject::Connection>   active_conns_;

    session_api*                     session_ = nullptr;
    load_controller*                 load_controller_          = nullptr;
    target_panel*                    target_panel_             = nullptr;

    QStackedWidget*                  files_stack_    = nullptr;
    QStackedWidget*                  kernel_stack_   = nullptr;
    QStackedWidget*                  console_stack_  = nullptr;
    QStackedWidget*                  wire_stack_     = nullptr;
    QStackedWidget*                  cp_stack_       = nullptr;
    file_explorer_panel*             idle_files_     = nullptr;
    kernel_explorer_panel*           idle_kernel_    = nullptr;
    console_panel*                   idle_console_   = nullptr;
    wire_log_panel*                  idle_wire_      = nullptr;
    cp_panel*                        idle_cp_        = nullptr;
    QPersistentModelIndex            active_target_;

    QList<QPersistentModelIndex>     probe_queue_;
    bool                             probing_ = false;
    bool                             probe_initiated_disconnect_ = false;

    QDockWidget*        target_dock_       = nullptr;
    QTabWidget*         explorer_tabs_     = nullptr;
    QToolBar*           main_toolbar_      = nullptr;
    QDockWidget*        wire_log_dock_     = nullptr;
    QDockWidget*        console_dock_      = nullptr;
    QLabel*             state_indicator_   = nullptr;
    QAction*            add_action_              = nullptr;
    QAction*            remove_action_           = nullptr;
    QAction*            edit_action_             = nullptr;
    QAction*            properties_action_       = nullptr;
    QActionGroup*       reset_mode_group_        = nullptr;
    QAction*            connect_action_          = nullptr;
    QAction*            disconnect_action_       = nullptr;
    QAction*            reset_action_            = nullptr;
    QAction*            power_on_action_         = nullptr;
    QAction*            power_off_action_        = nullptr;
    QAction*            power_off_force_action_  = nullptr;
    QAction*            wake_on_lan_action_      = nullptr;
    QAction*            load_executable_action_   = nullptr;
    QAction*            install_action_           = nullptr;
    QAction*            import_props_action_      = nullptr;
    QAction*            export_props_action_      = nullptr;

    QProgressDialog*    install_dialog_           = nullptr;
    QString             install_result_path_;
    void show_install_progress(const QString& file_name);
    void finish_install_progress(std::uint32_t lv2_status);
    void close_install_progress();
    QAction*            load_from_device_action_  = nullptr;

    QHash<QString, QAction*> named_actions_;
};

} // namespace opentm::tm_ui
