#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/screen_interactive.hpp>
#include "../editor/editor.hpp"
#include "../renderer/editor_renderer.hpp"

namespace sord {
namespace app {

class Application {
public:
    Application();
    explicit Application(std::filesystem::path initial_path);
    explicit Application(std::filesystem::path ipc_socket_path, bool use_ipc);
    ~Application();

    int run();

private:
    void mark_modified();
    void open_save_as_dialog();
    void close_save_as_dialog();
    void open_open_dialog();
    void close_open_dialog();
    void open_export_dialog();
    void close_export_dialog();
    void open_system_file_manager();
    bool load_document(const std::filesystem::path& path, std::string& error);
    void save_document();
    void save_document_as();
    void schedule_saved_status();

    [[nodiscard]] std::filesystem::path default_path() const;
    [[nodiscard]] std::string current_filename() const;
    [[nodiscard]] std::string current_status_text() const;
    [[nodiscard]] std::size_t visible_height() const;
    [[nodiscard]] std::size_t absolute_cursor_line_index() const;
    [[nodiscard]] std::size_t total_rendered_line_count() const;
    void ensure_cursor_visible();
    void scroll_view(std::ptrdiff_t delta);
    void enqueue_open_path(std::filesystem::path path);
    void process_pending_open_paths();
    void start_ipc_server(ftxui::ScreenInteractive* screen);
    void stop_ipc_server();

    std::shared_ptr<sord::editor::Editor> editor_;
    std::shared_ptr<sord::renderer::EditorRenderer> renderer_;

    std::filesystem::path current_path_;
    bool document_saved_ = false;
    bool document_modified_ = false;
    std::size_t editor_scroll_offset_ = 0;
    bool show_save_dialog_ = false;
    bool show_save_as_dialog_ = false;
    bool show_open_dialog_ = false;
    bool show_export_dialog_ = false;

    std::mutex pending_open_mutex_;
    std::vector<std::filesystem::path> pending_open_paths_;
    std::filesystem::path ipc_socket_path_;
    int ipc_listen_fd_ = -1;
    std::thread ipc_thread_;
    bool ipc_server_running_ = false;
    ftxui::ScreenInteractive* screen_ = nullptr;

    int title_bar_selected_ = 0;
    int save_as_tab_ = 0;

    std::string save_filename_;
    std::string save_as_path_;
    std::string open_file_path_;
    std::string export_path_;
    std::string export_error_message_;
    std::string open_error_message_;
    std::optional<std::chrono::steady_clock::time_point> saved_status_expires_;
};

}  // namespace app
}  // namespace sord
