#pragma once

#include "session_api.h"

#include <QString>

namespace opentm::tm_ui {

struct session_backend {
    enum class kind { in_process, remote_tcp, remote_local };

    kind    k    = kind::in_process;
    QString host = QStringLiteral("127.0.0.1");
    quint16 port = 4300;
    QString local_name;

    QString fallback_reason;
};

session_api* make_session(const session_backend& b, QObject* parent, QString* err);

} // namespace opentm::tm_ui
