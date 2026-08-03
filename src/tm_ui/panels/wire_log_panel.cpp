#include "wire_log_panel.h"

#include "../find_dialog.h"

#include <QAction>
#include <QColor>
#include <QIcon>
#include <QMenu>
#include <QPoint>
#include <QShortcut>
#include <QDateTime>
#include <QFont>
#include <QLineEdit>
#include <QList>
#include <QPlainTextEdit>
#include <QStringLiteral>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

namespace opentm::tm_ui {

namespace {

void highlight_matches(QPlainTextEdit* view, const QString& needle) {
    if (!view) return;
    QList<QTextEdit::ExtraSelection> selections;
    if (!needle.isEmpty()) {
        QTextCharFormat fmt;
        fmt.setBackground(QColor(255, 230, 0, 120));
        fmt.setForeground(QColor(0, 0, 0));
        QTextCursor cursor(view->document());
        while (true) {
            cursor = view->document()->find(needle, cursor);
            if (cursor.isNull()) break;
            QTextEdit::ExtraSelection sel;
            sel.cursor = cursor;
            sel.format = fmt;
            selections.append(sel);
        }
    }
    view->setExtraSelections(selections);
}

} // namespace

wire_log_panel::wire_log_panel(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    filter_ = new QLineEdit(this);
    filter_->setPlaceholderText(tr("Filter (highlights matches)"));
    filter_->setClearButtonEnabled(true);

    view_ = new QPlainTextEdit(this);
    view_->setReadOnly(true);
    view_->setMaximumBlockCount(5000);
    view_->setFont(QFont(QStringLiteral("Consolas"), 9));

    layout->addWidget(filter_);
    layout->addWidget(view_);

    connect(filter_, &QLineEdit::textChanged, this, [this](const QString& s) { highlight_matches(view_, s); });

    view_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view_, &QPlainTextEdit::customContextMenuRequested, this, &wire_log_panel::show_context_menu);

    auto* find_sc = new QShortcut(QKeySequence::Find, this);
    find_sc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(find_sc, &QShortcut::activated, this, &wire_log_panel::focus_find);
}

wire_log_panel::~wire_log_panel() = default;

QString wire_log_panel::text() const {
    return view_ ? view_->toPlainText() : QString();
}

void wire_log_panel::focus_find() {
    if (!find_) {
        find_ = new find_dialog([this] { return view_; }, this);
    }
    find_->focus_input(view_->textCursor().selectedText());
}

void wire_log_panel::show_context_menu(const QPoint& pos) {
    QMenu* menu = view_->createStandardContextMenu(view_->mapToGlobal(pos));
    menu->addSeparator();
    // Set the shortcut separately rather than through the addAction overload
    // that takes one - that overload is Qt 6.3, and the release bundle is
    // built against the oldest Qt we support.
    auto* find = menu->addAction(QIcon(QStringLiteral(":/icons/folder_explore.bmp")), tr("&Find..."), this, &wire_log_panel::focus_find);
    find->setShortcut(QKeySequence::Find);
    menu->addAction(QIcon(QStringLiteral(":/icons/delete.bmp")), tr("C&lear"), this, &wire_log_panel::clear);
    menu->addSeparator();
    menu->addAction(QIcon(QStringLiteral(":/icons/page_white_put.bmp")), tr("&Save to File..."), this, [this] { emit save_requested(); });
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(view_->mapToGlobal(pos));
}

void wire_log_panel::append(const QString& line) {
    append_at(line, QDateTime::currentDateTime());
}

void wire_log_panel::append_at(const QString& line, const QDateTime& when) {
    const auto ts = when.toString(QStringLiteral("HH:mm:ss.zzz"));

    if (line == last_line_ && repeat_count_ > 0) {
        ++repeat_count_;
        auto c = view_->textCursor();
        c.movePosition(QTextCursor::End);
        c.select(QTextCursor::LineUnderCursor);
        c.insertText(QStringLiteral("[%1] %2   (x%3)").arg(ts, line).arg(repeat_count_));
        view_->setTextCursor(c);
        view_->ensureCursorVisible();
        return;
    }

    last_line_    = line;
    repeat_count_ = 1;
    view_->appendPlainText(QStringLiteral("[%1] %2").arg(ts, line));
}

void wire_log_panel::clear() {
    view_->clear();
    last_line_.clear();
    repeat_count_ = 0;
}

} // namespace opentm::tm_ui
