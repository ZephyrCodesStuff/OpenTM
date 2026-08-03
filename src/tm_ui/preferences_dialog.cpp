#include "preferences_dialog.h"

#include "app_prefs.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace opentm::tm_ui {

preferences_dialog::preferences_dialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Preferences"));

    auto* tabs = new QTabWidget(this);
    tabs->addTab(build_general(), tr("&General"));
    tabs->addTab(build_console(), tr("&Console"));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { commit(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &preferences_dialog::restore_defaults);

    auto* outer = new QVBoxLayout(this);
    outer->addWidget(tabs);
    outer->addWidget(buttons);
    setMinimumWidth(470);
}

QWidget* preferences_dialog::build_general() {
    auto* page = new QWidget(this);
    auto* outer = new QVBoxLayout(page);

    auto* session_box = new QGroupBox(tr("Sessions"), page);
    auto* session_layout = new QVBoxLayout(session_box);

    use_tray_ = new QCheckBox(tr("Keep targets connected in the background"), page);
    use_tray_->setChecked(use_tray_supervisor());

    auto* tray_note = new QLabel(tr("Runs a tray icon and a local session server, so targets stay connected when this ""window closes and can be shared with other tools. Turn this off to keep everything ""inside this process; sessions then end with the window. Takes effect on restart."), page);
    tray_note->setWordWrap(true);
    tray_note->setEnabled(false);

    auto_reconnect_ = new QCheckBox(tr("Reconnect automatically after an unexpected drop"), page);
    auto_reconnect_->setChecked(auto_reconnect_enabled());

    session_layout->addWidget(use_tray_);
    session_layout->addWidget(tray_note);
    session_layout->addSpacing(6);
    session_layout->addWidget(auto_reconnect_);
    outer->addWidget(session_box);

    auto* targets_box = new QGroupBox(tr("Targets"), page);
    auto* targets_layout = new QVBoxLayout(targets_box);
    auto_arp_ = new QCheckBox(tr("Look up MAC addresses during a target search"), page);
    auto_arp_->setChecked(auto_arp_enabled());
    auto_arp_->setToolTip(tr("Needed for Wake-on-LAN. Only works on the local subnet."));
    targets_layout->addWidget(auto_arp_);
    outer->addWidget(targets_box);

    binary_units_ = new QCheckBox(tr("Use binary units (KiB, MiB) for file sizes"), page);
    binary_units_->setChecked(size_units_are_binary());
    binary_units_->setToolTip(tr("On: 1 KiB = 1024 bytes. Off: 1 kB = 1000 bytes."));
    outer->addWidget(binary_units_);

    outer->addStretch(1);
    return page;
}

QWidget* preferences_dialog::build_console() {
    auto* page = new QWidget(this);
    auto* outer = new QVBoxLayout(page);

    auto* form = new QFormLayout;
    font_combo_ = new QFontComboBox(page);
    font_combo_->setFontFilters(QFontComboBox::MonospacedFonts);
    font_combo_->setCurrentFont(QFont(console_font_family()));
    form->addRow(tr("&Font:"), font_combo_);

    font_size_ = new QSpinBox(page);
    font_size_->setRange(6, 32);
    font_size_->setSuffix(tr(" pt"));
    font_size_->setValue(console_font_point_size());
    form->addRow(tr("&Size:"), font_size_);
    outer->addLayout(form);

    preview_ = new QPlainTextEdit(page);
    preview_->setReadOnly(true);
    preview_->setPlainText(QStringLiteral(
        "[00:00:00.000] cellFsOpen(\"/app_home/EBOOT.BIN\") -> 0x0\n"
        "[00:00:00.017] lv2: process 0x01000500 started\n"
        "[00:00:00.042] 0123456789 iIlL1 oO0 {}[]()<> |/\\\n"));
    preview_->setFixedHeight(90);

    auto* preview_box = new QGroupBox(tr("Preview"), page);
    auto* preview_layout = new QVBoxLayout(preview_box);
    preview_layout->addWidget(preview_);
    outer->addWidget(preview_box);
    outer->addStretch(1);

    connect(font_combo_, &QFontComboBox::currentFontChanged, this, [this] { update_preview(); });
    connect(font_size_, qOverload<int>(&QSpinBox::valueChanged), this, [this] { update_preview(); });
    update_preview();
    return page;
}

void preferences_dialog::update_preview() {
    preview_->setFont(QFont(font_combo_->currentFont().family(), font_size_->value()));
}

void preferences_dialog::restore_defaults() {
    use_tray_->setChecked(true);
    auto_reconnect_->setChecked(true);
    auto_arp_->setChecked(true);
    binary_units_->setChecked(true);
    font_combo_->setCurrentFont(QFont(QStringLiteral("Consolas")));
    font_size_->setValue(10);
}

void preferences_dialog::commit() {
    set_use_tray_supervisor(use_tray_->isChecked());
    set_auto_reconnect_enabled(auto_reconnect_->isChecked());
    set_auto_arp_enabled(auto_arp_->isChecked());
    set_size_units_binary(binary_units_->isChecked());
    set_console_font(font_combo_->currentFont().family(), font_size_->value());
    emit applied();
}

} // namespace opentm::tm_ui
