#pragma once

#include <tm_launch/single_instance.h>

#include <QJsonObject>
#include <QObject>
#include <QString>

class QAction;
class QLocalSocket;
class QMenu;
class QProcess;
class QSystemTrayIcon;
class QTcpSocket;

namespace opentm::tm_tray {

class tray_supervisor : public QObject {
    Q_OBJECT
public:
    explicit tray_supervisor(QObject* parent = nullptr);
    ~tray_supervisor() override;

    void set_port(quint16 p)          { port_ = p; }
    void set_allow_mutating(bool on)  { allow_mutating_ = on; }

    bool start(bool open_gui_now);

private:
    void build_tray();
    void update_menu_state();

    bool start_server(QString* err);
    void claim_server_ownership();
    void stop_server();
    bool server_running() const;

    void open_gui();
    void show_about();
    void quit();

    QJsonObject server_call(const QString& method, const QJsonObject& params = {}, int timeout_ms = 800);
    void rebuild_targets_menu();

    void on_request(const QByteArray& line, QLocalSocket* peer);
    void reply(QLocalSocket* peer, const QJsonObject& obj);

    opentm::tm_launch::single_instance control_;
    QSystemTrayIcon* tray_     = nullptr;
    QProcess*        server_   = nullptr;
    QTcpSocket*      owner_link_ = nullptr;
    QAction*         start_action_ = nullptr;
    QAction*         stop_action_  = nullptr;
    QMenu*           targets_menu_ = nullptr;
    int              next_rpc_id_  = 100;
    quint16          port_          = 4300;
    bool             allow_mutating_ = true;
};

} // namespace opentm::tm_tray
