#include "app/application.hpp"
#include "app/save_manager.hpp"
#include "renderer/toolbar_renderer.hpp"
#include "renderer/menu/home_menu_renderer.hpp"
#include "renderer/menu/insert_menu_renderer.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "exporter/pdf_exporter.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace sord {
namespace app {

static std::filesystem::path normalize_ipc_socket_path(const std::filesystem::path& path) {
    std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }
    return path;
}

static void remove_socket_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        std::filesystem::remove(path, ec);
    }
}

Application::Application()
    : editor_(std::make_shared<sord::editor::Editor>()),
      renderer_(std::make_shared<sord::renderer::EditorRenderer>(editor_)),
      current_path_(default_path()),
      save_as_path_(current_path_.string()) {}

Application::Application(std::filesystem::path initial_path)
    : editor_(std::make_shared<sord::editor::Editor>()),
      renderer_(std::make_shared<sord::renderer::EditorRenderer>(editor_)),
      current_path_(std::move(initial_path)),
      save_as_path_(current_path_.string()) {
    std::string error;
    if (!current_path_.empty()) {
        load_document(current_path_, error);
    }
}

Application::Application(std::filesystem::path ipc_socket_path, bool use_ipc)
    : editor_(std::make_shared<sord::editor::Editor>()),
      renderer_(std::make_shared<sord::renderer::EditorRenderer>(editor_)),
      current_path_(default_path()),
      save_as_path_(current_path_.string()),
      ipc_socket_path_(normalize_ipc_socket_path(std::move(ipc_socket_path))) {
    if (use_ipc) {
        remove_socket_file(ipc_socket_path_);
    }
}

Application::~Application() {
    stop_ipc_server();
    remove_socket_file(ipc_socket_path_);
}

void Application::mark_modified() {
    document_modified_ = true;
    document_saved_ = false;
}

void Application::enqueue_open_path(std::filesystem::path path) {
    std::lock_guard<std::mutex> lock(pending_open_mutex_);
    pending_open_paths_.push_back(std::move(path));
}

void Application::process_pending_open_paths() {
    std::vector<std::filesystem::path> pending;
    {
        std::lock_guard<std::mutex> lock(pending_open_mutex_);
        pending.swap(pending_open_paths_);
    }
    for (auto& path : pending) {
        std::string error;
        if (load_document(path, error)) {
            open_file_path_ = current_path_.string();
            show_open_dialog_ = false;
        }
    }
}

void Application::start_ipc_server(ftxui::ScreenInteractive* screen) {
    if (ipc_server_running_ || ipc_socket_path_.empty()) {
        return;
    }

    ipc_server_running_ = true;
    screen_ = screen;
    ipc_thread_ = std::thread([this] {
        ipc_listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (ipc_listen_fd_ < 0) {
            ipc_server_running_ = false;
            return;
        }

        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::string path = ipc_socket_path_.string();
        if (path.size() >= sizeof(addr.sun_path)) {
            close(ipc_listen_fd_);
            ipc_server_running_ = false;
            return;
        }
        strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

        remove_socket_file(ipc_socket_path_);
        if (bind(ipc_listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close(ipc_listen_fd_);
            ipc_server_running_ = false;
            return;
        }

        if (listen(ipc_listen_fd_, 1) != 0) {
            close(ipc_listen_fd_);
            ipc_server_running_ = false;
            return;
        }

        while (ipc_server_running_) {
            int client = accept(ipc_listen_fd_, nullptr, nullptr);
            if (client < 0) {
                continue;
            }

            std::string buffer;
            buffer.reserve(1024);
            char ch;
            while (read(client, &ch, 1) == 1) {
                if (ch == '\n') {
                    break;
                }
                buffer.push_back(ch);
            }
            close(client);
            if (!buffer.empty()) {
                enqueue_open_path(std::filesystem::path(buffer));
                if (screen_) {
                    screen_->PostEvent(ftxui::Event::Custom);
                }
            }
        }
    });
}

void Application::stop_ipc_server() {
    if (!ipc_server_running_) {
        return;
    }
    ipc_server_running_ = false;
    if (ipc_listen_fd_ >= 0) {
        shutdown(ipc_listen_fd_, SHUT_RDWR);
        close(ipc_listen_fd_);
        ipc_listen_fd_ = -1;
    }
    if (ipc_thread_.joinable()) {
        ipc_thread_.join();
    }
}

void Application::open_save_as_dialog() {
    show_save_as_dialog_ = true;
    save_as_path_ = current_path_.string();
    save_as_tab_ = 0;
}

void Application::open_open_dialog() {
    show_open_dialog_ = true;
    open_file_path_ = current_path_.string();
    open_error_message_.clear();
}

void Application::close_open_dialog() {
    show_open_dialog_ = false;
}

void Application::close_save_as_dialog() {
    show_save_as_dialog_ = false;
}

void Application::open_export_dialog() {
    show_export_dialog_ = true;
    export_path_ = current_path_.replace_extension(".pdf").string();
    export_error_message_.clear();
}

void Application::close_export_dialog() {
    show_export_dialog_ = false;
}

bool Application::load_document(const std::filesystem::path& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        error = "Unable to open file.";
        return false;
    }

    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<std::string> lines;
    std::string current_line;
    for (char ch : contents) {
        if (ch == '\n') {
            lines.push_back(current_line);
            current_line.clear();
        } else if (ch == '\r') {
            continue;
        } else {
            current_line.push_back(ch);
        }
    }
    if (!current_line.empty() || contents.empty()) {
        lines.push_back(current_line);
    }

    if (lines.empty()) {
        lines.emplace_back();
    }

    editor_->document().set_title(path.filename().string());
    editor_->document().set_lines(std::move(lines));
    current_path_ = path;
    document_saved_ = true;
    document_modified_ = false;
    editor_scroll_offset_ = 0;
    return true;
}

std::size_t Application::visible_height() const {
    return 24;
}

std::size_t Application::absolute_cursor_line_index() const {
    std::size_t index = 0;
    const auto& pages = editor_->document().pages();
    const auto current_page = editor_->document().current_page();
    for (std::size_t page_index = 0; page_index < current_page; ++page_index) {
        index += pages[page_index].lines().size();
        index += 1; // separator line between pages
    }
    index += editor_->document().cursor_row();
    return index;
}

void Application::ensure_cursor_visible() {
    const auto cursor_line = absolute_cursor_line_index();
    const auto height = visible_height();
    if (cursor_line < editor_scroll_offset_) {
        editor_scroll_offset_ = cursor_line;
    } else if (cursor_line >= editor_scroll_offset_ + height) {
        editor_scroll_offset_ = cursor_line - height + 1;
    }
}

std::size_t Application::total_rendered_line_count() const {
    const auto& pages = editor_->document().pages();
    std::size_t total = 0;
    for (std::size_t page_index = 0; page_index < pages.size(); ++page_index) {
        if (page_index != 0) {
            total += 1;
        }
        total += pages[page_index].lines().size();
    }
    return total;
}

void Application::scroll_view(std::ptrdiff_t delta) {
    if (delta < 0) {
        const auto abs_delta = static_cast<std::size_t>(-delta);
        editor_scroll_offset_ = (abs_delta > editor_scroll_offset_) ? 0 : editor_scroll_offset_ - abs_delta;
        return;
    }

    const auto max_offset = total_rendered_line_count();
    const auto height = visible_height();
    if (max_offset <= height) {
        editor_scroll_offset_ = 0;
        return;
    }

    const auto max_allowed = max_offset - height;
    editor_scroll_offset_ = std::min(editor_scroll_offset_ + static_cast<std::size_t>(delta), max_allowed);
}

void Application::open_system_file_manager() {
    if (current_path_.empty()) {
        return;
    }

#ifdef __linux__
    std::string command = "xdg-open '" + current_path_.parent_path().string() + "'";
#elif defined(__APPLE__)
    std::string command = "open '" + current_path_.parent_path().string() + "'";
#else
    std::string command;
#endif
    if (!command.empty()) {
        std::system(command.c_str());
    }
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
    start_ipc_server(&screen);

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

    auto btn_open = Button("Open", [&] {
        open_open_dialog();
        open_error_message_.clear();
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

    auto btn_file_manager = Button("Open File Manager", [&] {
        open_system_file_manager();
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_toolbar_home = Button("Home", [&] {
        title_bar_selected_ = renderer::ToolbarRenderer::Tab::Home;
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_toolbar_insert = Button("Insert", [&] {
        title_bar_selected_ = renderer::ToolbarRenderer::Tab::Insert;
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_toolbar_draw = Button("Draw", [&] {
        title_bar_selected_ = renderer::ToolbarRenderer::Tab::Draw;
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto btn_toolbar_graph_charts = Button("Graph/Charts", [&] {
        title_bar_selected_ = renderer::ToolbarRenderer::Tab::GraphCharts;
        screen.PostEvent(Event::Custom);
    }, title_btn_option);

    auto title_bar_container = Container::Horizontal({
        btn_save,
        btn_save_as,
        btn_open,
        btn_new_page,
        btn_export,
        btn_file_manager,
    });

    auto title_bar = Renderer(title_bar_container, [&] {
        return hbox({
            text("Sord"),
            text("  "),
            btn_save->Render(),
            text(" "),
            btn_save_as->Render(),
            text(" "),
            btn_open->Render(),
            text(" "),
            btn_new_page->Render(),
            text(" "),
            btn_export->Render(),
            text(" "),
            btn_file_manager->Render(),
            filler(),
            text(current_filename()),
            text(" "),
            text(current_status_text()),
        });
    });

    auto toolbar_title_bar = Container::Horizontal({
        btn_toolbar_home,
        btn_toolbar_insert,
        btn_toolbar_draw,
        btn_toolbar_graph_charts,
    });

    auto toolbar = Renderer(toolbar_title_bar, [&] {
        std::string sub;
        if (title_bar_selected_ == sord::renderer::ToolbarRenderer::Tab::Home) {
            sub = sord::renderer::menu::HomeMenuRenderer::Render();
        } else if (title_bar_selected_ == sord::renderer::ToolbarRenderer::Tab::Insert) {
            sub = sord::renderer::menu::insertMenuRenderer::Render();
        } else {
            sub = sord::renderer::ToolbarRenderer::RenderSubToolbar(title_bar_selected_);
        }

        // Render tab row, then a separator line, then the menu content below it
        return vbox(std::vector<Element>{
            hbox({
                btn_toolbar_home->Render(),
                text(" "),
                btn_toolbar_insert->Render(),
                text(" "),
                btn_toolbar_draw->Render(),
                text(" "),
                btn_toolbar_graph_charts->Render(),
            }),
            // separator (wider to span full toolbar)
            text(std::string(260, '-')),
            // menu options rendered below the separator
            text(sub)
        }) | border;
    });

    auto editor_area = Renderer([&] {
        std::vector<Element> lines;
        for (const auto& line : renderer_->render_visible_lines(80, 24, editor_scroll_offset_)) {
            lines.push_back(text(line));
        }
        return vbox(lines) | border | flex;
    });

    auto status_bar = Renderer([&] {
        const auto& doc = editor_->document();
        const auto current_page = doc.current_page();
        const auto cursor_row = doc.cursor_row();
        const auto page_limit = doc.page_line_limit();

        std::ostringstream oss;
        oss << "Ln " << (cursor_row + 1)
            << ", Col " << (doc.cursor_column() + 1)
            << "  Page " << (current_page + 1);

        Element warning = text("");
        if (cursor_row + 1 >= page_limit) {
            warning = text("  [Page Full - Please add a new page]") | color(Color::Red);
        }

        return hbox({
            text(oss.str()),
            warning,
            filler(),
            text("UTF-8"),
            text("        "),
            text("INSERT")
        }) | border;
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

    auto open_input = Input(&open_file_path_, "Enter path...", InputOption::Default());
    auto open_btn_ok = Button("Open", [&] {
        open_error_message_.clear();
        try {
            std::filesystem::path filepath(open_file_path_);
            if (filepath.empty()) {
                open_error_message_ = "Please enter a path.";
            } else {
                if (!filepath.has_parent_path()) {
                    filepath = std::filesystem::current_path() / filepath;
                }
                if (!std::filesystem::exists(filepath)) {
                    open_error_message_ = "File does not exist.";
                } else {
                    std::ifstream in(filepath, std::ios::binary);
                    if (!in.is_open()) {
                        open_error_message_ = "Unable to open file.";
                    } else {
                        std::vector<std::string> lines;
                        std::string line;
                        while (std::getline(in, line)) {
                            lines.push_back(line);
                        }
                        if (lines.empty()) {
                            lines.emplace_back();
                        }
                        editor_->document().set_title(filepath.filename().string());
                        editor_->document().set_lines(std::move(lines));
                        current_path_ = filepath;
                        document_saved_ = true;
                        document_modified_ = false;
                        close_open_dialog();
                    }
                }
            }
        } catch (...) {
            open_error_message_ = "Invalid path format.";
        }
        screen.PostEvent(Event::Custom);
    });
    auto open_btn_cancel = Button("Cancel", [&] {
        close_open_dialog();
        screen.PostEvent(Event::Custom);
    });

    Component open_inner = Container::Vertical({
        open_input,
        Container::Horizontal({
            open_btn_ok,
            open_btn_cancel,
        }),
    });

    auto open_overlay = Renderer(open_inner, [&]() -> Element {
        std::vector<Element> items = {
            text("Path:"),
            open_input->Render(),
            text(""),
        };
        if (!open_error_message_.empty()) {
            items.push_back(text(open_error_message_) | color(Color::Red));
            items.push_back(text(""));
        }
        items.push_back(hbox({
            open_btn_ok->Render(),
            text(" "),
            open_btn_cancel->Render(),
        }));
        return window(text("Open File"), vbox(items)) | center;
    });

    auto open_overlay_maybe = Maybe(open_overlay, &show_open_dialog_);

    auto export_input = Input(&export_path_, "Enter path...", InputOption::Default());
    auto export_btn_ok = Button("Export", [&] {
        export_error_message_.clear();
        try {
            std::filesystem::path filepath(export_path_);
            if (filepath.extension().empty()) {
                filepath += ".pdf";
            } else {
                filepath.replace_extension(".pdf");
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
    auto root = Container::Stacked(std::vector<Component>{main_container, save_overlay_maybe, save_as_overlay_maybe, open_overlay_maybe, export_overlay_maybe});

    auto component = CatchEvent(root, [&](Event event) {
        process_pending_open_paths();
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

        if (show_open_dialog_) {
            if (event == Event::Escape) {
                close_open_dialog();
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
            ensure_cursor_visible();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::ArrowUp) {
            editor_->handle_input(259);
            ensure_cursor_visible();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::ArrowDown) {
            editor_->handle_input(258);
            ensure_cursor_visible();
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
                scroll_view(-1);
                screen.PostEvent(Event::Custom);
                return true;
            }
            if (mouse.button == Mouse::WheelDown) {
                scroll_view(1);
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
