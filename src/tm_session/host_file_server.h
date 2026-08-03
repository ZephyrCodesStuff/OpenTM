#pragma once

#include <tm_core/deci3_codec.h>
#include <tm_core/drfp_codec.h>

#include <QByteArray>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <unordered_map>

class QFile;

namespace opentm::tm_core { class tcp_connection; }

namespace opentm::tm_ui {

class host_file_server : public QObject {
    Q_OBJECT
public:
    explicit host_file_server(opentm::tm_core::tcp_connection* conn, QObject* parent = nullptr);
    ~host_file_server() override;
    void set_file_server_dir(const QString& dir);

    void authorise_transfer_path(const QString& host_path);
    void forget_transfer_path(const QString& host_path);
    static QString normalise_host_path(QString p);
    static bool looks_absolute(const QString& p);
    static QStringList transfer_path_chain(const QString& host_path);
    void set_cfw_dex(bool on) noexcept { cfw_dex_ = on; }
    QString file_server_dir() const noexcept { return file_server_dir_; }
    void reset();

public slots:
    void on_frame_received(opentm::tm_core::deci3_frame f);
signals:
    void log_message(QString line);
    void status_message(QString text);

private:
    QString resolve(const QString& kit_path) const;

    static bool is_synthetic_param_sfo(const QString& kit_path);

    QByteArray handle_init(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_stat(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_open(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_close(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_read(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_write(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_mkdir(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_rmdir(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_dopen(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_dclose(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_dread(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_ftruncate(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_truncate(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_rename(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_unlink(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_fstat(const opentm::tm_core::drfp_frame& f);
    QByteArray handle_seek(const opentm::tm_core::drfp_frame& f);

    void send_drfp_reply(const QByteArray& drfp_body);

    opentm::tm_core::tcp_connection* connection_ = nullptr;
    QString                          file_server_dir_;
    
    void warn_no_serving_dir();
    bool is_authorised(const QString& cleaned) const;
    bool warned_no_dir_ = false;

    // absolute host paths cleared for an in-flight transfer, plus ancestors
    QSet<QString> transfer_paths_;
    bool cfw_dex_    = false;

    std::unordered_map<std::uint32_t, QFile*> open_files_;
    std::uint32_t next_fd_ = 0x1000;

    struct synth_handle {
        QByteArray content;
        qint64     pos = 0;
    };

    std::unordered_map<std::uint32_t, synth_handle> synth_files_;

    struct dir_handle {
        QStringList entries;
        QString     local;
        int         pos = 0;
    };
    std::unordered_map<std::uint32_t, dir_handle> open_dirs_;
};

} // namespace opentm::tm_ui
