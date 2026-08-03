#include "session_factory.h"

#include "remote_session.h"
#include "target_session.h"

namespace opentm::tm_ui {

session_api* make_session(const session_backend& b, QObject* parent, QString* err) {
    if (b.k == session_backend::kind::in_process) {
        return new target_session(parent);
    }

    auto* s  = new remote_session(parent);
    const bool ok = (b.k == session_backend::kind::remote_tcp) ? s->attach_tcp(b.host, b.port) : s->attach_local(b.local_name);
    if (!ok) {
        if (err) {
            *err = (b.k == session_backend::kind::remote_tcp) ? QStringLiteral("could not reach opentm_server at %1:%2").arg(b.host).arg(b.port) : QStringLiteral("could not reach opentm_server on '%1'").arg(b.local_name);
        }
        delete s;
        return nullptr;
    }
    return s;
}

} // namespace opentm::tm_ui
