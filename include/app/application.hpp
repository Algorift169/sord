#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "editor/editor.hpp"
#include "renderer/editor_renderer.hpp"

namespace sord {
namespace app {

class Application {
public:
    Application();

    int run();

private:
    void mark_modified();
    void open_save_as_dialog();
    void close_save_as_dialog();
    void open_export_dialog();
    void close_export_dialog();
    void save_document();
    void save_document_as();
    void schedule_saved_status();

    [[nodiscard]] std::filesystem::path default_path() const;
    [[nodiscard]] std::string current_filename() const;
    [[nodiscard]] std::string current_status_text() const;

    std::shared_ptr<sord::editor::Editor> editor_;
    std::shared_ptr<sord::renderer::EditorRenderer> renderer_;

    std::filesystem::path current_path_;
    bool document_saved_ = false;
    bool document_modified_ = false;
    bool show_save_dialog_ = false;
    bool show_save_as_dialog_ = false;
    bool show_export_dialog_ = false;

    int title_bar_selected_ = 0;
    int save_as_tab_ = 0;

    std::string save_filename_;
    std::string save_as_path_;
    std::string export_path_;
    std::string export_error_message_;
    std::optional<std::chrono::steady_clock::time_point> saved_status_expires_;
};

}  // namespace app
}  // namespace sord
