#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;

namespace opentm::tm_ui {

class load_executable_dialog : public QDialog {
    Q_OBJECT
public:
    struct result {
        QString path;

        bool    is_device_path        = false;

        QString file_serving_dir;
        QString home_dir;
        QString cmdline;

        bool reset_target        = false;
        bool clear_streams       = true;
        bool enable_debug_module = false;
        bool disable_ppu_debug   = false;
        bool disable_spu_debug   = false;
    };

    explicit load_executable_dialog(const QString& default_app_home = {}, const QString& default_home = {}, QWidget* parent = nullptr);

    result selection() const;

    void save_state() const;
    void restore_state();

protected:
    void done(int result) override;

private slots:
    void on_device_toggled(bool checked);
    void on_app_home_use_elf_toggled(bool checked);
    void on_home_use_elf_toggled(bool checked);
    void on_browse_file();
    void on_browse_app_home();
    void on_browse_home();

private:
    QWidget*      host_file_row_   = nullptr;
    QWidget*      host_dirs_       = nullptr;
    QLineEdit*    file_path_edit_  = nullptr;
    QPushButton*  browse_btn_      = nullptr;

    QCheckBox*    device_check_    = nullptr;
    QComboBox*    device_combo_    = nullptr;
    QLineEdit*    device_path_edit_ = nullptr;

    QCheckBox*    app_home_check_  = nullptr;
    QLineEdit*    app_home_edit_   = nullptr;
    QPushButton*  app_home_browse_ = nullptr;
    QCheckBox*    app_home_use_elf_ = nullptr;

    QCheckBox*    home_check_      = nullptr;
    QLineEdit*    home_edit_       = nullptr;
    QPushButton*  home_browse_     = nullptr;
    QCheckBox*    home_use_elf_    = nullptr;

    QLineEdit*    cmdline_edit_    = nullptr;
    QCheckBox*    reset_check_     = nullptr;
    QCheckBox*    clear_streams_check_ = nullptr;
    QCheckBox*    enable_debug_check_  = nullptr;
    QCheckBox*    disable_ppu_check_   = nullptr;
    QCheckBox*    disable_spu_check_   = nullptr;
};

} // namespace opentm::tm_ui
