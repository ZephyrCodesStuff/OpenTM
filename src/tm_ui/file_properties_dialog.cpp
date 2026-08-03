#include "file_properties_dialog.h"

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTabWidget>
#include <QVBoxLayout>

namespace opentm::tm_ui {

namespace {

constexpr std::uint32_t kPermMask = 0777u;

QDateTimeEdit* make_time_edit(std::uint64_t secs, QWidget* parent) {
    auto* e = new QDateTimeEdit(parent);
    e->setDisplayFormat(QStringLiteral("dd/MM/yyyy HH:mm:ss"));
    e->setCalendarPopup(true);
    e->setDateTime(QDateTime::fromSecsSinceEpoch(static_cast<qint64>(secs)));
    return e;
}

} // namespace

file_properties_dialog::file_properties_dialog(
    const QString& kit_path, const values& current, QWidget* parent)
    : QDialog(parent), path_(kit_path), current_(current)
{
    setWindowTitle(tr("Properties"));

    auto* perms_page = new QWidget(this);
    {
        auto* outer = new QVBoxLayout(perms_page);
        auto* box = new QGroupBox(tr("Owner Permissions"), perms_page);
        auto* v = new QVBoxLayout(box);
        read_  = new QCheckBox(tr("Read"), perms_page);
        write_ = new QCheckBox(tr("Write"), perms_page);
        exec_  = new QCheckBox(tr("Execute"), perms_page);
        const auto owner = (current.mode >> 6) & 7u;
        read_->setChecked(owner & 4u);
        write_->setChecked(owner & 2u);
        exec_->setChecked(owner & 1u);
        v->addWidget(read_);
        v->addWidget(write_);
        v->addWidget(exec_);
        outer->addWidget(box);

        auto* form = new QFormLayout;
        numeric_ = new QLineEdit(perms_page);
        numeric_->setMaximumWidth(70);
        numeric_->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[0-7xX]{0,3}")), numeric_));
        numeric_->setText(QString::number(current.mode & kPermMask, 8).rightJustified(3, QLatin1Char('0')));
        form->addRow(tr("Numeric Value:"), numeric_);
        outer->addLayout(form);

        auto* note = new QLabel(tr("Use 'x' to keep the original value for that digit, or set the owner ""digit from the checkboxes above."), perms_page);
        note->setWordWrap(true);
        outer->addWidget(note);
        outer->addStretch(1);

        for (auto* b : { read_, write_, exec_ }) {
            connect(b, &QCheckBox::toggled, this, [this] { sync_numeric_from_boxes(); });
        }
        connect(numeric_, &QLineEdit::textEdited, this, [this] { sync_boxes_from_numeric(); });
    }

    auto* times_page = new QWidget(this);
    {
        auto* form = new QFormLayout(times_page);

        auto* created = make_time_edit(current.ctime, times_page);
        created->setEnabled(false);
        form->addRow(tr("Created Time:"), created);

        auto* arow = new QWidget(times_page);
        auto* ah = new QVBoxLayout(arow);
        ah->setContentsMargins(0, 0, 0, 0);
        set_atime_ = new QCheckBox(tr("Set accessed time"), times_page);
        atime_ = make_time_edit(current.atime, times_page);
        atime_->setEnabled(false);
        ah->addWidget(set_atime_);
        ah->addWidget(atime_);
        form->addRow(tr("Accessed Time:"), arow);

        auto* mrow = new QWidget(times_page);
        auto* mh = new QVBoxLayout(mrow);
        mh->setContentsMargins(0, 0, 0, 0);
        set_mtime_ = new QCheckBox(tr("Set modified time"), times_page);
        mtime_ = make_time_edit(current.mtime, times_page);
        mtime_->setEnabled(false);
        mh->addWidget(set_mtime_);
        mh->addWidget(mtime_);
        form->addRow(tr("Modified Time:"), mrow);

        connect(set_atime_, &QCheckBox::toggled, atime_, &QWidget::setEnabled);
        connect(set_mtime_, &QCheckBox::toggled, mtime_, &QWidget::setEnabled);

        auto* note = new QLabel(tr("Times are in the host timezone."), times_page);
        note->setEnabled(false);
        form->addRow(QString(), note);
    }

    auto* tabs = new QTabWidget(this);
    tabs->addTab(perms_page, tr("&Permissions"));
    tabs->addTab(times_page, tr("&Time Settings"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* outer = new QVBoxLayout(this);
    auto* what = new QLabel(path_, this);
    what->setWordWrap(true);
    outer->addWidget(what);
    outer->addWidget(tabs);
    outer->addWidget(buttons);
}

void file_properties_dialog::sync_numeric_from_boxes() {
    if (syncing_) return;
    syncing_ = true;
    auto text = numeric_->text().rightJustified(3, QLatin1Char('0'));
    const int owner = (read_->isChecked() ? 4 : 0) | (write_->isChecked() ? 2 : 0) | (exec_->isChecked() ? 1 : 0);
    text[0] = QLatin1Char(static_cast<char>('0' + owner));
    numeric_->setText(text);
    syncing_ = false;
}

void file_properties_dialog::sync_boxes_from_numeric() {
    if (syncing_) return;
    syncing_ = true;
    const auto text = numeric_->text().rightJustified(3, QLatin1Char('0'));
    const QChar c = text[0];
    if (c.isDigit()) {
        const int owner = c.digitValue();
        read_->setChecked(owner & 4);
        write_->setChecked(owner & 2);
        exec_->setChecked(owner & 1);
    }
    syncing_ = false;
}

std::uint32_t file_properties_dialog::permission_bits() const {
    const auto original = current_.mode & kPermMask;
    const auto text = numeric_->text().rightJustified(3, QLatin1Char('0'));
    std::uint32_t bits = 0;
    for (int i = 0; i < 3; ++i) {
        const int shift = (2 - i) * 3;
        const QChar c = text[i];
        // 'x' keeps whatever that digit already was
        const std::uint32_t digit = (c == QLatin1Char('x') || c == QLatin1Char('X')) ? ((original >> shift) & 7u) : static_cast<std::uint32_t>(c.digitValue() & 7);
        bits |= digit << shift;
    }
    return bits;
}

file_properties_dialog::values file_properties_dialog::result() const {
    values v = current_;
    v.mode  = (current_.mode & ~kPermMask) | permission_bits();
    v.atime = set_atime_->isChecked() ? static_cast<std::uint64_t>(atime_->dateTime().toSecsSinceEpoch()) : current_.atime;
    v.mtime = set_mtime_->isChecked() ? static_cast<std::uint64_t>(mtime_->dateTime().toSecsSinceEpoch()) : current_.mtime;
    return v;
}

} // namespace opentm::tm_ui
