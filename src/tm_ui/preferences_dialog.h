#pragma once

#include <QDialog>

class QCheckBox;
class QFontComboBox;
class QPlainTextEdit;
class QSpinBox;

namespace opentm::tm_ui {

class preferences_dialog : public QDialog {
    Q_OBJECT
public:
    explicit preferences_dialog(QWidget* parent = nullptr);

signals:
    void applied();

private:
    QWidget* build_general();
    QWidget* build_console();
    void update_preview();
    void restore_defaults();
    void commit();

    QCheckBox*      use_tray_       = nullptr;
    QCheckBox*      auto_reconnect_ = nullptr;
    QCheckBox*      auto_arp_       = nullptr;
    QCheckBox*      binary_units_   = nullptr;
    QFontComboBox*  font_combo_     = nullptr;
    QSpinBox*       font_size_      = nullptr;
    QPlainTextEdit* preview_        = nullptr;
};

} // namespace opentm::tm_ui
