#pragma once

#include <tm_session/target_record.h>

#include <tm_core/target_type.h>

#include <QDialog>
#include <QString>

class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace opentm::tm_ui {


class add_target_dialog : public QDialog {
    Q_OBJECT
public:
    explicit add_target_dialog(QWidget* parent = nullptr);

    void set_record(const target_record& r);

    target_record record() const;

private slots:
    void on_type_changed(int index);

private:
    QLineEdit* name_edit_            = nullptr;
    QComboBox* type_combo_           = nullptr;
    QLineEdit* host_edit_            = nullptr;
    QSpinBox*  port_spin_            = nullptr;
    QLineEdit* mac_edit_             = nullptr;
    QLineEdit* file_server_dir_edit_ = nullptr;
    QLineEdit* home_dir_edit_        = nullptr;

    // record() edits this rather than building a fresh one, so properties the
    // dialog does not show (load options, timeouts, reset mode) survive an edit
    target_record base_;
    bool       port_follows_type_ = true;
};

} // namespace opentm::tm_ui
