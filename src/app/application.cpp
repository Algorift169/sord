#include "app/application.hpp"
#include "app/save_manager.hpp"

#include <chrono>
#include <sstream>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "exporter/pdf_exporter.hpp"

namespace sord {
namespace app {

Application::Application()
    : editor_(std::make_shared<sord::editor::Editor>()),
      renderer_(std::make_shared<sord::renderer::EditorRenderer>(editor_)),
      current_path_(default_path()),
      save_as_path_(current_path_.string()) {}

void Application::mark_modified() {
    document_modified_ = true;
    document_saved_ = false;
}

void Application::open_save_as_dialog() {
    show_save_as_dialog_ = true;
    save_as_path_ = current_path_.string();
    save_as_tab_ = 0;
}

void Application::open_export_dialog() {
    show_export_dialog_ = true;
    export_path_ = current_path_.replace_extension(".pdf").string();
    export_error_message_.clear();
}

void Application::close_export_dialog() {
    show_export_dialog_ = false;
}

void Application::close_save_as_dialog() {
    show_save_as_dialog_ = false;
}

void Application::schedule_saved_status() {
    saved_status_expires_ = std::chrono::steady_clock::now() + std::chrono::seconds(2);
}

std::filesystem::path Application::default_path() const {
    return std::filesystem::current_path() / "Untitled.sord";
}

std::string Application::current_filename() const {
    return current_path_.filename().string();
}

std::string Application::current_status_text() const {
    if (saved_status_expires_) {
        if (std::chrono::steady_clock::now() < *saved_status_expires_) {
            return "[Saved]";
        }
        return "[Ready]";
    }
    return "[Ready]";
}

void Application::save_document() {
    if (current_path_.empty()) {
        save_document_as();
        return;
    }

    std::string error;
    if (SaveManager::save(editor_->document(), current_path_, error)) {
        document_saved_ = true;
        document_modified_ = false;
        editor_->document().set_title(current_filename());
        schedule_saved_status();
    } else {
        // In milestone 2, errors are not surfaced beyond leaving state as ready.
    }
}

void Application::save_document_as() {
    auto normalized = SaveManager::normalize_path(save_as_path_);
    std::string error;
    if (SaveManager::save(editor_->document(), normalized, error)) {
        current_path_ = normalized;
        document_saved_ = true;
        document_modified_ = false;
        editor_->document().set_title(current_filename());
        schedule_saved_status();
        close_save_as_dialog();
    }
}

int Application::run() {
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();

    std::string save_error_message_;

    auto title_btn_option = ButtonOption::Animated();
    title_btn_option.transform = [](const EntryState& s) {
        auto element = text("[" + s.label + "]");
        if (s.focused) {
            element = element | inverted;
        }
        return element;
    };

    auto btn_save = Button("Save", [&] {
        if (document_saved_) {
            save_document();
        } else {
            show_save_dialog_ = true;
            save_filename_ = current_filename();
            save_error_message_ = "";
        }
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_save_as = Button("Save As", [&] {
        open_save_as_dialog();
        save_error_message_ = "";
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_export = Button("Export PDF", [&] {
        open_export_dialog();
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_new_page = Button("+", [&] {
        editor_->document().add_page();
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto title_bar_container = Container::Horizontal({
        btn_save,
        btn_save_as,
        btn_new_page,
        btn_export,
    });

    auto title_bar = Renderer(title_bar_container, [&] {
        return hbox({
            text("Sord"),
            text("  "),
            btn_save->Render(),
            text(" "),
            btn_save_as->Render(),
            text(" "),
            text(" "),
            btn_new_page->Render(),
            text(" "),
            btn_export->Render(),
            filler(),
            text(current_filename()),
            text(" "),
            text(current_status_text()),
        }) | border;
    });

    auto toolbar = Renderer([&] {
        return text(renderer_->render_toolbar()) | border;
    });

    auto editor_area = Renderer([&] {
        std::vector<Element> lines;
        for (const auto& line : renderer_->render_visible_lines(80, 24)) {
            lines.push_back(text(line));
        }
        return vbox(lines) | border | flex;
    });

    auto status_bar = Renderer([&] {
        return text(renderer_->render_status_bar()) | border;
    });

    auto save_input = Input(&save_filename_, "Enter filename...", InputOption::Default());
    auto save_btn_ok = Button("Save", [&] {
        save_error_message_ = "";
        auto filepath = std::filesystem::current_path() / SaveManager::normalize_path(save_filename_);
        std::string error;
        try {
            if (!filepath.has_parent_path() || !std::filesystem::exists(filepath.parent_path())) {
                save_error_message_ = "Directory does not exist.";
            } else {
                bool old_file_exists = document_saved_ && !current_path_.empty() && std::filesystem::exists(current_path_);
                std::filesystem::path old_path = current_path_;

                if (SaveManager::save(editor_->document(), filepath, error)) {
                    if (old_file_exists && old_path != filepath) {
                        std::error_code ec;
                        std::filesystem::remove(old_path, ec);
                    }
                    current_path_ = filepath;
                    document_saved_ = true;
                    document_modified_ = false;
                    editor_->document().set_title(current_filename());
                    schedule_saved_status();
                    show_save_dialog_ = false;
                } else {
                    save_error_message_ = error.empty() ? "Save failed." : error;
                }
            }
        } catch (...) {
            save_error_message_ = "Invalid path format.";
        }
        screen.PostEvent(Event::Custom);
    });
    auto save_btn_cancel = Button("Cancel", [&] {
        show_save_dialog_ = false;
        screen.PostEvent(Event::Custom);
    });

    Component save_inner = Container::Vertical({
        save_input,
        Container::Horizontal({
            save_btn_ok,
            save_btn_cancel,
        }),
    });

    auto save_overlay = Renderer(save_inner, [&]() -> Element {
        std::vector<Element> items = {
            text("Filename:"),
            save_input->Render(),
            text(""),
        };
        if (!save_error_message_.empty()) {
            items.push_back(text(save_error_message_) | color(Color::Red));
            items.push_back(text(""));
        }
        items.push_back(hbox({
            save_btn_ok->Render(),
            text(" "),
            save_btn_cancel->Render(),
        }));
        return window(text("Save"), vbox(items)) | center;
    });

    auto save_overlay_maybe = Maybe(save_overlay, &show_save_dialog_);

    auto save_as_input = Input(&save_as_path_, "Enter path...", InputOption::Default());
    auto save_as_btn_save = Button("Save", [&] {
        save_error_message_ = "";
        auto filepath = SaveManager::normalize_path(save_as_path_);
        std::string error;
        try {
            if (!filepath.has_parent_path() || !std::filesystem::exists(filepath.parent_path())) {
                save_error_message_ = "Directory does not exist.";
            } else {
                bool old_file_exists = document_saved_ && !current_path_.empty() && std::filesystem::exists(current_path_);
                std::filesystem::path old_path = current_path_;

                if (SaveManager::save(editor_->document(), filepath, error)) {
                    if (old_file_exists && old_path != filepath) {
                        std::error_code ec;
                        std::filesystem::remove(old_path, ec);
                    }
                    current_path_ = filepath;
                    document_saved_ = true;
                    document_modified_ = false;
                    editor_->document().set_title(current_filename());
                    schedule_saved_status();
                    close_save_as_dialog();
                } else {
                    save_error_message_ = error.empty() ? "Save failed." : error;
                }
            }
        } catch (...) {
            save_error_message_ = "Invalid path format.";
        }
        screen.PostEvent(Event::Custom);
    });
    auto save_as_btn_cancel = Button("Cancel", [&] {
        close_save_as_dialog();
        screen.PostEvent(Event::Custom);
    });

    Component save_as_inner = Container::Vertical({
        save_as_input,
        Container::Horizontal({
            save_as_btn_save,
            save_as_btn_cancel,
        }),
    });

    auto save_as_overlay = Renderer(save_as_inner, [&]() -> Element {
        std::vector<Element> items = {
            text("Path:"),
            save_as_input->Render(),
            text(""),
        };
        if (!save_error_message_.empty()) {
            items.push_back(text(save_error_message_) | color(Color::Red));
            items.push_back(text(""));
        }
        items.push_back(hbox({
            save_as_btn_save->Render(),
            text(" "),
            save_as_btn_cancel->Render(),
        }));
        return window(text("Save As"), vbox(items)) | center;
    });

    auto save_as_overlay_maybe = Maybe(save_as_overlay, &show_save_as_dialog_);

    auto export_input = Input(&export_path_, "Enter path...", InputOption::Default());
    auto export_btn_ok = Button("Export", [&] {
        export_error_message_.clear();
        try {
            std::filesystem::path filepath = SaveManager::normalize_path(export_path_);
            if (filepath.extension().empty()) {
                filepath = filepath;
                filepath += ".pdf";
            }

            if (!filepath.has_parent_path() || !std::filesystem::exists(filepath.parent_path())) {
                export_error_message_ = "Directory does not exist.";
            } else {
                std::string error;
                sord::exporter::PDFExporter exporter;
                if (exporter.Export(editor_->document(), filepath, error)) {
                    close_export_dialog();
                } else {
                    export_error_message_ = error.empty() ? "Export failed." : error;
                }
            }
        } catch (...) {
            export_error_message_ = "Invalid path format.";
        }
        screen.PostEvent(Event::Custom);
    });
    auto export_btn_cancel = Button("Cancel", [&] {
        close_export_dialog();
        screen.PostEvent(Event::Custom);
    });

    Component export_inner = Container::Vertical({
        export_input,
        Container::Horizontal({
            export_btn_ok,
            export_btn_cancel,
        }),
    });

    auto export_overlay = Renderer(export_inner, [&]() -> Element {
        std::vector<Element> items = {
            text("Path:"),
            export_input->Render(),
            text(""),
        };
        if (!export_error_message_.empty()) {
            items.push_back(text(export_error_message_) | color(Color::Red));
            items.push_back(text(""));
        }
        items.push_back(hbox({
            export_btn_ok->Render(),
            text(" "),
            export_btn_cancel->Render(),
        }));
        return window(text("Export PDF"), vbox(items)) | center;
    });

    auto export_overlay_maybe = Maybe(export_overlay, &show_export_dialog_);

    auto main_container = Container::Vertical(std::vector<Component>{title_bar, toolbar, editor_area, status_bar});
    auto root = Container::Stacked(std::vector<Component>{main_container, save_overlay_maybe, save_as_overlay_maybe, export_overlay_maybe});

    auto component = CatchEvent(root, [&](Event event) {
        if (show_save_dialog_) {
            if (event == Event::Escape) {
                show_save_dialog_ = false;
                screen.PostEvent(Event::Custom);
                return true;
            }
            return false;
        }

        if (show_save_as_dialog_) {
            if (event == Event::Escape) {
                close_save_as_dialog();
                screen.PostEvent(Event::Custom);
                return true;
            }
            return false;
        }

        if (show_export_dialog_) {
            if (event == Event::Escape) {
                close_export_dialog();
                screen.PostEvent(Event::Custom);
                return true;
            }
            return false;
        }

        if (event == Event::CtrlS) {
            if (document_saved_) {
                save_document();
            } else {
                show_save_dialog_ = true;
                save_filename_ = current_filename();
            }
            screen.PostEvent(Event::Custom);
            return true;
        }

        if (event.is_character()) {
            editor_->handle_char(event.character());
            mark_modified();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::Backspace || event == Event::Delete) {
            editor_->handle_input(127);
            mark_modified();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::Return) {
            editor_->handle_input(10);
            mark_modified();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::ArrowUp) {
            editor_->handle_input(259);
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::ArrowDown) {
            editor_->handle_input(258);
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::ArrowLeft) {
            editor_->handle_input(260);
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::ArrowRight) {
            editor_->handle_input(261);
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event.is_mouse()) {
            const auto& mouse = event.mouse();
            if (mouse.button == Mouse::WheelUp) {
                editor_->handle_input(259);
                screen.PostEvent(Event::Custom);
                return true;
            }
            if (mouse.button == Mouse::WheelDown) {
                editor_->handle_input(258);
                screen.PostEvent(Event::Custom);
                return true;
            }
        }
        if (event == Event::Escape) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(component);
    return 0;
}

}  // namespace app
}  // namespace sord
