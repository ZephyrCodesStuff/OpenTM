#pragma once

#include <tm_session/session_api.h>

#include <QHash>
#include <QList>
#include <QVariantList>
#include <QWidget>

#include <cstdint>

class QPushButton;
class QStandardItem;
class QStandardItemModel;
class QTableWidget;
class QTreeView;
class QLabel;

namespace opentm::tm_ui {

class kernel_explorer_panel : public QWidget {
    Q_OBJECT
public:
    explicit kernel_explorer_panel(QWidget* parent = nullptr);
    ~kernel_explorer_panel() override;

public slots:
    void clear();

    // hook the controller signals to these slots in main_window
    void on_process_list_ready(QList<session_api::process_summary> processes);
    void on_threads_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::ppu_thread_info> threads);
    void on_modules_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::prx_info> modules);
    void on_mutexes_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::mutex_info> entries);
    void on_lwmutexes_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::lwmutex_info> entries);
    void on_conds_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::cond_info> entries);
    void on_event_queues_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::event_queue_info> entries);
    void on_containers_ready(std::uint32_t pid, QList<opentm::tm_core::dbgp::container_info> entries);

signals:
    void refresh_requested();
    void threads_requested(std::uint32_t pid);
    void modules_requested(std::uint32_t pid);
    void mutexes_requested(std::uint32_t pid);
    void lwmutexes_requested(std::uint32_t pid);
    void conds_requested(std::uint32_t pid);
    void event_queues_requested(std::uint32_t pid);
    void containers_requested(std::uint32_t pid);
    void core_dump_requested(std::uint32_t pid);
    void resume_requested(std::uint32_t pid);
    void pause_requested(std::uint32_t pid);
    void terminate_requested(std::uint32_t pid);

private slots:
    void on_refresh_clicked();
    void on_tree_expanded(const QModelIndex& idx);
    void on_tree_selection_changed();

private:
    QStandardItem* find_or_create_category(QStandardItem* process_item, const QString& key);
    void populate_detail_table(std::uint32_t pid, const QString& category, int highlight_row);
    void populate_property_table(const QVariantList& attrs);
    bool resolve_selection(std::uint32_t& pid_out, QString& category_out, int& row_out) const;

    QPushButton*        refresh_btn_      = nullptr;
    QPushButton*        copy_value_btn_   = nullptr;
    QPushButton*        core_dump_btn_    = nullptr;
    QPushButton*        resume_btn_       = nullptr;
    QPushButton*        pause_btn_        = nullptr;
    QPushButton*        terminate_btn_    = nullptr;
    QLabel*             status_label_     = nullptr;
    QTreeView*          tree_             = nullptr;
    QStandardItemModel* model_            = nullptr;
    QTableWidget*       detail_           = nullptr;

    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::ppu_thread_info>> threads_cache_;
    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::mutex_info>> mutexes_cache_;
    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::lwmutex_info>> lwmutexes_cache_;
    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::cond_info>> conds_cache_;
    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::event_queue_info>> evqs_cache_;
    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::prx_info>> modules_cache_;
    QHash<std::uint32_t, QList<opentm::tm_core::dbgp::container_info>> containers_cache_;
    
    std::uint32_t current_process_pid() const;
};

} // namespace opentm::tm_ui
