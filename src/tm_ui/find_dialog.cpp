#include "find_dialog.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

namespace opentm::tm_ui {

find_dialog::find_dialog(view_source source, QWidget* parent) : QDialog(parent), source_(std::move(source))
{
    setWindowTitle(tr("Find"));
    setModal(false);

    needle_ = new QLineEdit(this);
    auto* find_row = new QHBoxLayout;
    find_row->addWidget(new QLabel(tr("Fi&nd what:"), this));
    find_row->addWidget(needle_, 1);

    match_case_ = new QCheckBox(tr("Match &case"), this);
    whole_word_ = new QCheckBox(tr("Match &whole word only"), this);
    wrap_       = new QCheckBox(tr("Wra&p around"), this);
    wrap_->setChecked(true);

    auto* dir_box = new QGroupBox(tr("Direction"), this);
    up_   = new QRadioButton(tr("&Up"), dir_box);
    down_ = new QRadioButton(tr("&Down"), dir_box);
    down_->setChecked(true);
    auto* dir_layout = new QHBoxLayout(dir_box);
    dir_layout->addWidget(up_);
    dir_layout->addWidget(down_);

    find_btn_  = new QPushButton(tr("&Find Next"), this);
    find_btn_->setDefault(true);
    auto* close = new QPushButton(tr("Close"), this);

    status_ = new QLabel(this);
    status_->setStyleSheet(QStringLiteral("color:#c0392b;"));

    auto* opts = new QVBoxLayout;
    opts->addWidget(match_case_);
    opts->addWidget(whole_word_);
    opts->addWidget(wrap_);

    auto* buttons = new QVBoxLayout;
    buttons->addWidget(find_btn_);
    buttons->addWidget(close);
    buttons->addStretch(1);

    auto* grid = new QGridLayout;
    grid->addLayout(find_row, 0, 0, 1, 2);
    grid->addLayout(opts,     1, 0);
    grid->addWidget(dir_box,  1, 1);
    grid->addLayout(buttons,  0, 2, 2, 1);

    auto* outer = new QVBoxLayout(this);
    outer->addLayout(grid);
    outer->addWidget(status_);

    connect(find_btn_, &QPushButton::clicked, this, [this] { find_next(down_->isChecked()); });
    connect(close, &QPushButton::clicked, this, &QDialog::hide);
    connect(needle_, &QLineEdit::returnPressed, this, [this] { find_next(down_->isChecked()); });
    connect(needle_, &QLineEdit::textChanged, this, [this] { status_->clear(); });

    // f3
    // shift+f3 while the dialog has focus
    auto* next = new QShortcut(QKeySequence::FindNext, this);
    connect(next, &QShortcut::activated, this, [this] { find_next(true); });
    auto* prev = new QShortcut(QKeySequence::FindPrevious, this);
    connect(prev, &QShortcut::activated, this, [this] { find_next(false); });
}

void find_dialog::focus_input(const QString& preset) {
    if (!preset.isEmpty()) needle_->setText(preset);
    show();
    raise();
    activateWindow();
    needle_->setFocus();
    needle_->selectAll();
}

void find_dialog::find_next(bool forwards) {
    auto* view = source_ ? source_() : nullptr;
    if (!view) {
        status_->setText(tr("Nothing to search."));
        return;
    }
    const QString needle = needle_->text();
    if (needle.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (!forwards)                flags |= QTextDocument::FindBackward;
    if (match_case_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (whole_word_->isChecked()) flags |= QTextDocument::FindWholeWords;

    QTextCursor found = view->document()->find(needle, view->textCursor(), flags);
    if (found.isNull() && wrap_->isChecked()) {
        QTextCursor from(view->document());
        if (!forwards) from.movePosition(QTextCursor::End);
        found = view->document()->find(needle, from, flags);
    }

    if (found.isNull()) {
        status_->setText(tr("Cannot find \"%1\".").arg(needle));
        return;
    }
    status_->clear();
    view->setTextCursor(found);
    view->ensureCursorVisible();
}

} // namespace opentm::tm_ui
