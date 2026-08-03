#include "tray_supervisor.h"

#include <tm_launch/launch.h>
#include <tm_ui/about_dialog.h>
#include <tm_ui/app_style.h>

#include <QAction>
#include <QApplication>
#include <QFont>
#include <QHostAddress>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QMenu>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTcpSocket>

namespace opentm::tm_tray {

using opentm::tm_launch::single_instance;

tray_supervisor::tray_supervisor(QObject* parent) : QObject(parent) {}

tray_supervisor::~tray_supervisor() {
    stop_server();
}

bool tray_supervisor::start(bool open_gui_now) {
    if (!control_.acquire(QLatin1String(opentm::tm_launch::supervisor_socket))) {
        return false;
    }
    connect(&control_, &single_instance::request_received, this, &tray_supervisor::on_request);

    build_tray();

    QString err;
    if (!start_server(&err)) {
        tray_->showMessage(tr("OpenTM"), tr("Could not start the session server: %1").arg(err), QSystemTrayIcon::Warning);
    }
    if (open_gui_now) open_gui();
    return true;
}

void tray_supervisor::build_tray() {
    tray_ = new QSystemTrayIcon(QIcon(QStringLiteral(":/icons/computer.bmp")), this);
    tray_->setToolTip(tr("OpenTM"));

    auto* menu = new QMenu;

    auto* open = menu->addAction(tr("Open Target Manager..."));
    QFont bold = open->font();
    bold.setBold(true);
    open->setFont(bold);
    connect(open, &QAction::triggered, this, &tray_supervisor::open_gui);

    auto* about = menu->addAction(tr("About OpenTM..."));
    connect(about, &QAction::triggered, this, &tray_supervisor::show_about);

    menu->addSeparator();
    targets_menu_ = menu->addMenu(tr("Targets"));
    targets_menu_->setIcon(QIcon(QStringLiteral(":/icons/computer.bmp")));
    connect(targets_menu_, &QMenu::aboutToShow, this, &tray_supervisor::rebuild_targets_menu);

    menu->addSeparator();
    start_action_ = menu->addAction(tr("Start server"));
    connect(start_action_, &QAction::triggered, this, [this] {
        QString err;
        if (!start_server(&err)) {
            tray_->showMessage(tr("OpenTM"), err, QSystemTrayIcon::Warning);
        }
    });
    stop_action_ = menu->addAction(tr("Stop server"));
    connect(stop_action_, &QAction::triggered, this, &tray_supervisor::stop_server);

    menu->addSeparator();
    connect(menu->addAction(tr("Exit")), &QAction::triggered, this, &tray_supervisor::quit);

    tray_->setContextMenu(menu);
    connect(tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::DoubleClick) open_gui();
    });
    tray_->show();
    update_menu_state();
}

void tray_supervisor::update_menu_state() {
    const bool up = server_running();
    if (start_action_) start_action_->setEnabled(!up);
    if (stop_action_)  stop_action_->setEnabled(up);
    if (tray_) {
        tray_->setToolTip(up ? tr("OpenTM - server on port %1").arg(port_) : tr("OpenTM - server stopped"));
    }
}

bool tray_supervisor::server_running() const {
    return server_ && server_->state() != QProcess::NotRunning;
}

bool tray_supervisor::start_server(QString* err) {
    if (server_running()) return true;

    const auto exe = opentm::tm_launch::sibling_executable(
        QStringLiteral("opentm_server"));
    if (exe.isEmpty()) {
        if (err) *err = tr("opentm_server not found next to the tray binary");
        return false;
    }

    delete server_;
    server_ = new QProcess(this);
    server_->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(server_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        update_menu_state();
        if (code != 0 && tray_) {
            tray_->showMessage(tr("OpenTM"), tr("The session server exited (code %1). ""Console sessions are gone.").arg(code), QSystemTrayIcon::Warning);
        }
    });

    QStringList args{QStringLiteral("--port"), QString::number(port_), QStringLiteral("--quiet"), QStringLiteral("--exit-with-owner")};
    if (allow_mutating_) args << QStringLiteral("--allow-mutating");

    server_->start(exe, args);
    if (!server_->waitForStarted(5000)) {
        if (err) *err = server_->errorString();
        update_menu_state();
        return false;
    }
    claim_server_ownership();
    update_menu_state();
    return true;
}

void tray_supervisor::claim_server_ownership() {
    delete owner_link_;
    owner_link_ = new QTcpSocket(this);
    connect(owner_link_, &QTcpSocket::connected, this, [this] {
        owner_link_->write(
            QJsonDocument(QJsonObject{{"id", 1}, {"method", "server.own"}})
                .toJson(QJsonDocument::Compact));
        owner_link_->write("\n");
    });
    owner_link_->connectToHost(QHostAddress::LocalHost, port_);
}

void tray_supervisor::stop_server() {
    if (owner_link_) {
        owner_link_->disconnect(this);
        owner_link_->abort();
        owner_link_->deleteLater();
        owner_link_ = nullptr;
    }
    if (!server_running()) return;
    server_->terminate();
    if (!server_->waitForFinished(4000)) server_->kill();
    update_menu_state();
}

QJsonObject tray_supervisor::server_call(const QString& method, const QJsonObject& params, int timeout_ms)
{
    if (!server_running()) return {};

    QTcpSocket sock;
    sock.connectToHost(QHostAddress::LocalHost, port_);
    if (!sock.waitForConnected(timeout_ms)) return {};

    QJsonObject req{{"id", next_rpc_id_++}, {"method", method}};
    if (!params.isEmpty()) req["params"] = params;
    sock.write(QJsonDocument(req).toJson(QJsonDocument::Compact));
    sock.write("\n");
    if (!sock.waitForBytesWritten(timeout_ms)) return {};

    QByteArray buf;
    while (!buf.contains('\n')) {
        if (!sock.waitForReadyRead(timeout_ms)) return {};
        buf += sock.readAll();
        // events can arrive before our reply; skip them
        int nl;
        while ((nl = buf.indexOf('\n')) >= 0) {
            const auto line = buf.left(nl);
            buf.remove(0, nl + 1);
            const auto obj = QJsonDocument::fromJson(line).object();
            if (obj.contains(QStringLiteral("event"))) continue;
            return obj;
        }
    }
    return {};
}

void tray_supervisor::rebuild_targets_menu() {
    targets_menu_->clear();

    const auto reply = server_call(QStringLiteral("target.list"));
    const auto targets = reply.value("result").toObject().value("targets").toArray();
    if (!reply.value("ok").toBool()) {
        targets_menu_->addAction(tr("(server not reachable)"))->setEnabled(false);
        return;
    }
    if (targets.isEmpty()) {
        targets_menu_->addAction(tr("(no open sessions)"))->setEnabled(false);
        return;
    }

    for (const auto& v : targets) {
        const auto t = v.toObject();
        const auto handle = t.value("target").toString();
        const auto name   = t.value("name").toString(handle);
        const bool ready  = t.value("session_ready").toBool();
        const bool up     = t.value("connected").toBool();

        auto* sub = targets_menu_->addMenu(QStringLiteral("%1  (%2:%3)").arg(name, t.value("host").toString()).arg(t.value("port").toInt()));
        sub->setIcon(QIcon(up ? QStringLiteral(":/icons/server_connect.bmp") : QStringLiteral(":/icons/computer.bmp")));

        auto* state = sub->addAction(ready ? tr("Session ready") : up ? tr("Connected") : tr("Disconnected"));
        state->setEnabled(false);
        sub->addSeparator();

        const struct { const char* label; const char* method; bool needs_link; } kOps[] = {
            {QT_TR_NOOP("Reset"),          "target.reset",      false},
            {QT_TR_NOOP("Power On"),       "target.power_on",   false},
            {QT_TR_NOOP("Shutdown"),       "target.power_off",  false},
            {QT_TR_NOOP("Shutdown (Force)"),"target.power_kill", false},
            {QT_TR_NOOP("Wake on LAN"),    "target.wake_on_lan", false},
        };
        for (const auto& op : kOps) {
            auto* a = sub->addAction(tr(op.label));
            a->setEnabled(!op.needs_link || up);
            const QString method = QLatin1String(op.method);
            connect(a, &QAction::triggered, this, [this, method, handle, name] {
                QJsonObject params{{"target", handle}};
                if (method == QLatin1String("target.reset")) {
                    params["mode"] = QStringLiteral("current");
                }
                const auto r = server_call(method, params, 3000);
                if (!r.value("ok").toBool()) {
                    tray_->showMessage(tr("OpenTM"), tr("%1 on %2 failed: %3").arg(method, name, r.value("error").toString(tr("no reply"))), QSystemTrayIcon::Warning);
                }
            });
        }

        sub->addSeparator();
        auto* disc = sub->addAction(tr("Disconnect"));
        disc->setEnabled(up);
        connect(disc, &QAction::triggered, this, [this, handle] {
            server_call(QStringLiteral("target.disconnect"), {{"target", handle}}, 3000);
        });
    }
}

void tray_supervisor::open_gui() {
    if (!single_instance::request(QLatin1String(opentm::tm_launch::gui_socket), "raise", 1500).isEmpty()) {
        return;
    }
    const auto exe = opentm::tm_launch::sibling_executable(
        QStringLiteral("opentm_app"));
    if (exe.isEmpty()) {
        tray_->showMessage(tr("OpenTM"), tr("opentm_app not found next to the tray binary"), QSystemTrayIcon::Warning);
        return;
    }
    QProcess::startDetached(exe, {});
}

void tray_supervisor::show_about() {
    opentm::tm_ui::about_dialog dlg;
    dlg.exec();
}

void tray_supervisor::quit() {
    single_instance::request(QLatin1String(opentm::tm_launch::gui_socket), "quit", 3000);
    stop_server();
    if (tray_) tray_->hide();
    QCoreApplication::quit();
}

void tray_supervisor::reply(QLocalSocket* peer, const QJsonObject& obj) {
    peer->write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    peer->write("\n");
}

void tray_supervisor::on_request(const QByteArray& line, QLocalSocket* peer) {
    const auto method =
        QJsonDocument::fromJson(line).object().value("method").toString();

    if (method == QLatin1String("status")) {
        reply(peer, {{"ok", true}, {"server_running", server_running()}, {"port", port_}});
    } else if (method == QLatin1String("ensure_server")) {
        QString err;
        const bool ok = start_server(&err);
        reply(peer, ok ? QJsonObject{{"ok", true}, {"port", port_}} : QJsonObject{{"ok", false}, {"error", err}});
    } else if (method == QLatin1String("stop_server")) {
        stop_server();
        reply(peer, {{"ok", true}});
    } else if (method == QLatin1String("restyle")) {
        opentm::tm_ui::apply_saved_style();
        reply(peer, {{"ok", true}});
    } else if (method == QLatin1String("open_gui")) {
        open_gui();
        reply(peer, {{"ok", true}});
    } else if (method == QLatin1String("quit")) {
        reply(peer, {{"ok", true}});
        peer->flush();
        quit();
    } else {
        reply(peer, {{"ok", false}, {"error", QStringLiteral("unknown method '%1'").arg(method)}});
    }
}

} // namespace opentm::tm_tray
