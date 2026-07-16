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
#include <ftxui/screen/box.hpp>
#include "../editor/editor.hpp"
#include "../renderer/editor_renderer.hpp"
#include "renderer/toolbar_renderer.hpp"
#include "../layout/page_layout.hpp"

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
    
    // Clipboard and selection methods
    void copy_selection_to_clipboard();
    void paste_from_clipboard();
    void cut_selection_to_clipboard();
    bool map_mouse_to_position(int mx, int my, sord::editor::Document::Position& pos_out);
    bool map_absolute_line_to_page_row(std::size_t absolute_line, std::size_t& page_out, std::size_t& row_out) const;

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

    renderer::ToolbarRenderer::Tab title_bar_selected_ = renderer::ToolbarRenderer::Tab::Home;
    int save_as_tab_ = 0;

    std::string save_filename_;
    std::string save_as_path_;
    std::string open_file_path_;
    std::string export_path_;
    std::string export_error_message_;
    std::string open_error_message_;
    std::optional<std::chrono::steady_clock::time_point> saved_status_expires_;
    
    // Selection and clipboard members
    bool show_context_menu_ = false;
    int context_menu_x_ = 0;
    int context_menu_y_ = 0;
    bool is_selecting_ = false;
    std::string internal_clipboard_;
    ftxui::Box editor_box_;
    std::chrono::steady_clock::time_point last_click_time_;
    sord::editor::Document::Position last_click_position_;
    int click_count_ = 0;

    // Font family dropdown members
    bool show_font_dropdown_ = false;
    int font_dropdown_scroll_ = 0;
};

}  // namespace app
}  // namespace sord
