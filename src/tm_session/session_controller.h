
#pragma once

#include <tm_core/tsmp_codec.h>

#include <tm_core/target_type.h>
#include <tm_core/tcp_connection.h>

#include <QObject>
#include <QString>

#include <cstdint>

namespace opentm::tm_ui {

class session_controller : public QObject {
    Q_OBJECT
public:
    explicit session_controller(opentm::tm_core::tcp_connection* conn, QObject* parent = nullptr);
    ~session_controller() override;
    void set_target_type(opentm::tm_core::target_type t) noexcept {
        target_type_ = t;
    }
    opentm::tm_core::target_type target_type() const noexcept {
        return target_type_;
    }
    bool is_cfw_dex() const noexcept {
        return target_type_ == opentm::tm_core::target_type::cfw_dex;
    }

    std::uint32_t dbgshl_sb() const noexcept {
        return is_cfw_dex() ? 0x00100000u : 0x02100000u;
    }
    std::uint32_t drfp_sb() const noexcept {
        return is_cfw_dex() ? 0x00000000u : 0x02000000u;
    }
    bool          is_ready()      const noexcept { return session_ready_; }
    opentm::tm_core::tsmp_boot_mode current_boot_mode() const noexcept { return boot_mode_; }
    std::uint16_t session_token() const noexcept { return session_token_; }
    std::uint16_t sub_token()     const noexcept { return sub_token_; }

    std::uint32_t next_dbgshl_seq() noexcept { return dbgshl_seq_++; }
    bool is_dbgshl_stream_open() const noexcept { return dbgshl_stream_open_; }
    void mark_dbgshl_stream_open() noexcept { dbgshl_stream_open_ = true; }

public slots:
    void on_connection_state(opentm::tm_core::tcp_connection::state s);
    void on_version_string(const QString& version);
    void on_session_token(std::uint16_t token);
    void on_sub_token(std::uint16_t sub);
    void on_handshake_acked();
    void on_session_reaped(std::uint16_t reaped_token);
    void on_server_rejection(std::uint8_t kind);
    void on_debug_agent_up();
    void on_boot_param(quint64 value, bool in_effect);
    void on_target_came_up();
    void set_control_only(bool on) noexcept { control_only_ = on; }
    bool control_only() const noexcept { return control_only_; }
    void on_dex_tsmp_reply(std::uint16_t reply_cmd);
    void on_tty_text(const QString& text);

signals:
    void log_message(QString line);
    void status_message(QString text);
    void state_indicator_changed(opentm::tm_core::tcp_connection::state s);
    void session_ready(std::uint16_t token, std::uint16_t sub_token);
    void session_invalidated();
    void debug_agent_ready();
    void boot_mode_changed(QString name);

    void sdk_version_received(QString sdk);

public:
    void send_warmup_deregister();

private:
    void send_version_probe();
    void send_warmup();
    void send_session_init();
    void send_dex_tsmp(std::uint16_t cmd, const QByteArray& body, const QString& tag);

    opentm::tm_core::tcp_connection* connection_         = nullptr;
    std::uint16_t                    session_token_      = 0;
    std::uint16_t                    sub_token_          = 0;
    opentm::tm_core::tsmp_boot_mode  boot_mode_ = opentm::tm_core::tsmp_boot_mode::unknown;
    bool                             control_only_       = false;
    bool                             session_ready_      = false;
    std::uint32_t                    dbgshl_seq_         = 0x1000;
    opentm::tm_core::target_type     target_type_ =
        opentm::tm_core::target_type::decr_tcp;
    bool                             dbgshl_stream_open_ = false;
    bool                             sdk_emitted_        = false;
};

} // namespace opentm::tm_ui
