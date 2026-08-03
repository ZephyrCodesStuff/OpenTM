#pragma once

#include <QDialog>

#include <functional>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;

namespace opentm::tm_ui {

// modeless search over a QPlainTextEdit
// the target is resolved lazily so the console can keep searching whichever channel tab is in front
class find_dialog : public QDialog {
    Q_OBJECT
public:
    using view_source = std::function<QPlainTextEdit*()>;

    find_dialog(view_source source, QWidget* parent = nullptr);

    void focus_input(const QString& preset = {});

private:
    void find_next(bool forwards);

    view_source     source_;
    QLineEdit*      needle_    = nullptr;
    QCheckBox*      match_case_ = nullptr;
    QCheckBox*      whole_word_ = nullptr;
    QRadioButton*   up_        = nullptr;
    QRadioButton*   down_      = nullptr;
    QCheckBox*      wrap_      = nullptr;
    QLabel*         status_    = nullptr;
    QPushButton*    find_btn_  = nullptr;
};

} // namespace opentm::tm_ui
