#pragma once

#include <tm_core/dfmp_codec.h>
#include <tm_core/tcp_connection.h>

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>

#include <cstdint>
#include <vector>

namespace opentm::tm_ui {

class session_controller;

class file_explorer_controller : public QObject {
    Q_OBJECT
public:
    file_explorer_controller(opentm::tm_core::tcp_connection* conn, session_controller* sess, QObject* parent = nullptr);
    ~file_explorer_controller() override;

public slots:
    void list_directory(const QString& path);
    void download_file(const QString& kit_path, const QString& host_path, std::uint32_t size);
    void upload_file(const QString& host_path, const QString& kit_path);
    void delete_file(const QString& kit_path);
    void rename_file(const QString& from, const QString& to);
    void set_serving_dir(const QString& dir) { serving_dir_ = dir; }
    void make_directory(const QString& kit_path, std::uint32_t mode);
    void set_permissions(const QString& kit_path, std::uint32_t mode);
    void set_times(const QString& kit_path, std::uint64_t atime, std::uint64_t mtime);

    void on_dfmp_get_entries_reply(std::uint32_t seq, QByteArray body);
    void on_dfmp_op_reply(std::uint32_t seq, std::uint32_t marker, std::uint32_t status);
    void on_session_invalidated();

signals:
    void log_message(QString line);
    void status_message(QString text);
    void directory_listed(QString path, std::vector<opentm::tm_core::dfmp_file_entry> entries);
    void file_op_finished(QString op, quint32 status);

private:
    std::uint32_t send_dfmp(std::uint32_t cmd, const QByteArray& body, const QString& what);

    opentm::tm_core::tcp_connection* connection_      = nullptr;
    session_controller*               session_         = nullptr;
    QString                           pending_path_;
    std::uint32_t                     open_seq_        = 0;
    std::uint32_t                     list_seq_        = 0;
    QHash<std::uint32_t, QString>     pending_ops_;
    QString                           serving_dir_;
};

} // namespace opentm::tm_ui
