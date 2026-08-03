#pragma once

#include <tm_core/dfmp_codec.h>

#include <QStringList>
#include <QWidget>
#include <QString>

#include <vector>

class QLineEdit;
class QMouseEvent;
class QPushButton;
class QTableView;
class QStandardItemModel;

namespace opentm::tm_ui {

class file_explorer_panel : public QWidget {
    Q_OBJECT
public:
    explicit file_explorer_panel(QWidget* parent = nullptr);
    ~file_explorer_panel() override;
    QString current_path() const;

public slots:
    void clear();
    void show_directory(const QString& path, std::vector<opentm::tm_core::dfmp_file_entry> entries);

signals:
    void list_directory_requested(QString path);
    void log_message(QString line);
    void run_requested(QString kit_path);
    void download_requested(QString kit_path, quint32 size);
    void upload_requested(QString kit_dir);
    void delete_requested(QString kit_path, bool is_dir);
    void properties_requested(QString kit_path, quint32 mode, quint64 atime, quint64 mtime, quint64 ctime);
    void rename_requested(QString kit_path, QString new_name);
    void make_dir_requested(QString kit_dir, QString name);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void show_context_menu(const QPoint& pos);
    QString absolute_path_for(int row) const;
    void navigate_to(const QString& path);
    void record_history(const QString& path);
    void go_back();
    void go_forward();
    void update_nav_buttons();

    QLineEdit*          path_edit_   = nullptr;
    QTableView*         view_        = nullptr;
    QStandardItemModel* model_       = nullptr;
    QString             current_path_;
    QPushButton*        back_btn_    = nullptr;
    QPushButton*        fwd_btn_     = nullptr;
    QStringList         history_;
    int                 history_pos_ = -1;
    bool                in_history_nav_ = false;
};

} // namespace opentm::tm_ui
