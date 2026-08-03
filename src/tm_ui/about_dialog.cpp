#include "about_dialog.h"

#include <build_info.h>

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>
#include <QSysInfo>
#include <QVBoxLayout>

namespace opentm::tm_ui {

namespace {

// One block, plain text, so a bug report can carry it verbatim.
QString version_report() {
    return QStringLiteral(
        "OpenTM %1 (%2, branch %3, %4)\n"
        "Qt %5 (built against %6)\n"
        "%7 / %8\n"
        "%9")
        .arg(QStringLiteral(OPENTM_VERSION), QStringLiteral(OPENTM_GIT_HASH), QStringLiteral(OPENTM_GIT_BRANCH), QStringLiteral(OPENTM_COMMIT_DATE), QString::fromLatin1(qVersion()), QStringLiteral(QT_VERSION_STR), QStringLiteral(OPENTM_COMPILER), QStringLiteral(OPENTM_BUILD_TYPE), QSysInfo::prettyProductName());
}

} // namespace

about_dialog::about_dialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("About OpenTM"));

    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel(QStringLiteral("<h2>OpenTM</h2>"), this);
    layout->addWidget(title);

    auto* version = new QLabel(this);
    version->setTextFormat(Qt::RichText);
    version->setTextInteractionFlags(Qt::TextSelectableByMouse);
    version->setText(QStringLiteral("<pre style='margin:0'>%1</pre>").arg(version_report().toHtmlEscaped()));
    layout->addWidget(version);

    auto* body = new QLabel(this);
    body->setTextFormat(Qt::RichText);
    body->setOpenExternalLinks(true);
    body->setWordWrap(true);
    body->setText(tr(
        "<p>An independent, clean-room implementation of a PlayStation&nbsp;3 "
        "target manager, speaking the DECI3 protocol over TCP.</p>"

        "<p>OpenTM is an independent project. It is not affiliated with, "
        "endorsed by, or derived from Sony Interactive Entertainment or its "
        "subsidiaries, including SN Systems.</p>"

        "<p>&quot;PlayStation&quot;, &quot;PS3&quot; and &quot;ProDG&quot; "
        "are trademarks of their respective owners, used here only to "
        "identify the hardware and software OpenTM interoperates with.</p>"

        "<p>Developed by <a href=\"https://github.com/sagemono\">"
        "sagemono</a> "

        "<h4>Attributions</h4>"
        "<p><b>FamFamFam Silk icon set</b> by Mark James - "
        "<a href=\"http://www.famfamfam.com/lab/icons/silk/\">"
        "famfamfam.com/lab/icons/silk</a> "
        "Licensed under "
        "<a href=\"https://creativecommons.org/licenses/by/2.5/\">"
        "Creative Commons Attribution 2.5</a></p>"
        "<p><b>Qt 6</b> - LGPLv3. "
        "<br>"
        "<b>Catch2</b> - Boost Software License 1.0.</p>"));
    layout->addWidget(body);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto* copy = buttons->addButton(tr("&Copy Version Info"), QDialogButtonBox::ActionRole);
    connect(copy, &QPushButton::clicked, this, [copy] {
        QGuiApplication::clipboard()->setText(version_report());
        copy->setText(tr("Copied"));
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(buttons);

    setMinimumWidth(460);
}

} // namespace opentm::tm_ui
