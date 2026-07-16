#include "app/application.hpp"
#include "app/save_manager.hpp"
#include "renderer/toolbar_renderer.hpp"
#include "renderer/menu/home_menu_renderer.hpp"
#include "renderer/menu/insert_menu_renderer.hpp"

#include <chrono>
#include <cstdio>
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
    screen.TrackMouse(true);
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
            sub = sord::renderer::menu::HomeMenuRenderer::Render(
                editor_->document().current_font_family()
            );
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
        auto lines = renderer_->render_visible_lines(80, 24, editor_scroll_offset_);
        return vbox(std::move(lines)) | border | flex | reflect(editor_box_);
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

    auto context_btn_option = ButtonOption::Animated();
    context_btn_option.transform = [](const EntryState& s) {
        return text("[" + s.label + "]");
    };

    auto context_menu_btn_copy = Button("Copy", [&] {
        copy_selection_to_clipboard();
        show_context_menu_ = false;
        screen.PostEvent(Event::Custom);
    }, context_btn_option);

    auto context_menu_btn_cut = Button("Cut", [&] {
        cut_selection_to_clipboard();
        show_context_menu_ = false;
        screen.PostEvent(Event::Custom);
    }, context_btn_option);

    auto context_menu_btn_paste = Button("Paste", [&] {
        paste_from_clipboard();
        show_context_menu_ = false;
        screen.PostEvent(Event::Custom);
    }, context_btn_option);

    auto context_menu_btn_select_all = Button("Select All", [&] {
        is_selecting_ = false;
        editor_->document().select_all();
        show_context_menu_ = false;
        screen.PostEvent(Event::Custom);
    }, context_btn_option);

    auto context_menu_btn_undo = Button("Undo", [&] {
        editor_->document().undo();
        show_context_menu_ = false;
        mark_modified();
        screen.PostEvent(Event::Custom);
    }, context_btn_option);

    auto context_menu_btn_redo = Button("Redo", [&] {
        editor_->document().redo();
        show_context_menu_ = false;
        mark_modified();
        screen.PostEvent(Event::Custom);
    }, context_btn_option);

    Component context_menu_inner = Container::Vertical(Components{
        context_menu_btn_copy,
        context_menu_btn_cut,
        context_menu_btn_paste,
        context_menu_btn_select_all,
        context_menu_btn_undo,
        context_menu_btn_redo,
    });

    auto context_menu_overlay = Renderer(context_menu_inner, [&]() -> Element {
        std::vector<Element> rows;
        for (int i = 0; i < context_menu_y_; ++i) {
            rows.push_back(text(""));
        }
        
        Element menu_box = window(text("Actions"), vbox(Elements{
            context_menu_btn_copy->Render(),
            context_menu_btn_cut->Render(),
            context_menu_btn_paste->Render(),
            context_menu_btn_select_all->Render(),
            context_menu_btn_undo->Render(),
            context_menu_btn_redo->Render(),
        }));
        
        rows.push_back(hbox({
            text(std::string(context_menu_x_, ' ')),
            menu_box
        }));
        
        return vbox(rows);
    });

    auto context_menu_overlay_maybe = Maybe(context_menu_overlay, &show_context_menu_);

    // Font dropdown overlay - positioned over the editor area
    auto font_dropdown_overlay = Renderer([&]() -> Element {
        std::vector<Element> rows;
        // Position dropdown below the toolbar/menu area (around row 6)
        for (int i = 0; i < 6; ++i) {
            rows.push_back(text(""));
        }
        
        auto fonts = renderer::menu::HomeMenuRenderer::GetFontList();
        const int visible_count = 15;  // Show more fonts at once
        int start = std::max(0, font_dropdown_scroll_);
        int end = std::min(static_cast<int>(fonts.size()), start + visible_count);
        
        Elements font_items;
        
        // Add scroll up indicator
        if (start > 0) {
            font_items.push_back(text("  ⬆ " + std::to_string(start) + " more above"));
        }
        
        // Add font items
        for (int i = start; i < end; ++i) {
            std::string mark = (fonts[i] == editor_->document().current_font_family()) ? "✓ " : "  ";
            font_items.push_back(text(mark + fonts[i]));
        }
        
        // Add scroll down indicator
        if (end < static_cast<int>(fonts.size())) {
            int remaining = static_cast<int>(fonts.size()) - end;
            font_items.push_back(text("  ⬇ " + std::to_string(remaining) + " more below"));
        }
        
        Element dropdown_box = window(text("Fonts (↑↓ to scroll)"), vbox(std::move(font_items)));
        rows.push_back(dropdown_box);
        
        return vbox(rows);
    });

    auto font_dropdown_overlay_maybe = Maybe(font_dropdown_overlay, &show_font_dropdown_);

    auto main_container = Container::Vertical(std::vector<Component>{title_bar, toolbar, editor_area, status_bar});
    auto root = Container::Stacked(std::vector<Component>{
        main_container,
        save_overlay_maybe,
        save_as_overlay_maybe,
        open_overlay_maybe,
        export_overlay_maybe,
        context_menu_overlay_maybe,
        font_dropdown_overlay_maybe
    });

    auto component = CatchEvent(root, [&](Event event) {
        process_pending_open_paths();
        
        if (show_context_menu_) {
            if (event == Event::Escape) {
                show_context_menu_ = false;
                screen.PostEvent(Event::Custom);
                return true;
            }
            if (event.is_mouse()) {
                const auto& mouse = event.mouse();
                if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
                    int menu_w = 16;
                    int menu_h = 6;
                    if (mouse.x < context_menu_x_ || mouse.x >= context_menu_x_ + menu_w ||
                        mouse.y < context_menu_y_ || mouse.y >= context_menu_y_ + menu_h) {
                        show_context_menu_ = false;
                        screen.PostEvent(Event::Custom);
                        return true;
                    }
                }
            }
        }

        // Handle font dropdown events
        if (show_font_dropdown_) {
            if (event == Event::Escape) {
                show_font_dropdown_ = false;
                font_dropdown_scroll_ = 0;
                screen.PostEvent(Event::Custom);
                return true;
            }
            if (event == Event::ArrowUp) {
                if (font_dropdown_scroll_ > 0) {
                    --font_dropdown_scroll_;
                    screen.PostEvent(Event::Custom);
                }
                return true;
            }
            if (event == Event::ArrowDown) {
                auto fonts = renderer::menu::HomeMenuRenderer::GetFontList();
                int max_scroll = static_cast<int>(fonts.size()) - 15;  // Match visible_count
                if (max_scroll < 0) max_scroll = 0;
                if (font_dropdown_scroll_ < max_scroll) {
                    ++font_dropdown_scroll_;
                    screen.PostEvent(Event::Custom);
                }
                return true;
            }
            if (event == Event::Return) {
                auto fonts = renderer::menu::HomeMenuRenderer::GetFontList();
                // Select the first visible font
                int idx = std::max(0, font_dropdown_scroll_);
                if (idx < static_cast<int>(fonts.size())) {
                    editor_->document().set_current_font_family(fonts[idx]);
                }
                show_font_dropdown_ = false;
                font_dropdown_scroll_ = 0;
                screen.PostEvent(Event::Custom);
                return true;
            }
            if (event.is_mouse()) {
                const auto& mouse = event.mouse();
                if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
                    auto fonts = renderer::menu::HomeMenuRenderer::GetFontList();
                    // Check if click is within dropdown area (rows 6-22 for 15 fonts + header)
                    if (mouse.y >= 6 && mouse.y <= 22 && mouse.x >= 0 && mouse.x <= 40) {
                        // Calculate which font was clicked
                        int visible_row = mouse.y - 7;  // Account for header row
                        int font_idx = font_dropdown_scroll_ + visible_row;
                        if (font_idx >= 0 && font_idx < static_cast<int>(fonts.size())) {
                            editor_->document().set_current_font_family(fonts[font_idx]);
                        }
                        show_font_dropdown_ = false;
                        font_dropdown_scroll_ = 0;
                        screen.PostEvent(Event::Custom);
                        return true;
                    } else {
                        // Click outside dropdown - close it
                        show_font_dropdown_ = false;
                        font_dropdown_scroll_ = 0;
                        screen.PostEvent(Event::Custom);
                        return false;
                    }
                }
            }
            // Block all other input when dropdown is open
            return true;
        } else {
            // Open the font dropdown when the user clicks the visible font selector label
            if (event.is_mouse() && title_bar_selected_ == renderer::ToolbarRenderer::Tab::Home) {
                const auto& mouse = event.mouse();
                if (mouse.button == Mouse::Left && mouse.motion == Mouse::Pressed) {
                    // The Home menu label appears on the first content row beneath the toolbar.
                    // Keep the hit area slightly generous so the interaction feels natural.
                    if (mouse.y >= 3 && mouse.y <= 5 && mouse.x >= 0 && mouse.x <= 18) {
                        show_font_dropdown_ = true;
                        font_dropdown_scroll_ = 0;
                        screen.PostEvent(Event::Custom);
                        return true;
                    }
                }
            }
        }

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

        if (event == Event::CtrlA) {
            is_selecting_ = false;
            editor_->document().select_all();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::CtrlZ) {
            editor_->document().undo();
            mark_modified();
            ensure_cursor_visible();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::CtrlY) {
            editor_->document().redo();
            mark_modified();
            ensure_cursor_visible();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::CtrlC) {
            copy_selection_to_clipboard();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::CtrlV) {
            paste_from_clipboard();
            mark_modified();
            ensure_cursor_visible();
            screen.PostEvent(Event::Custom);
            return true;
        }
        if (event == Event::CtrlX) {
            cut_selection_to_clipboard();
            screen.PostEvent(Event::Custom);
            return true;
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
            if (mouse.button == Mouse::Left) {
                if (mouse.motion == Mouse::Pressed) {
                    sord::editor::Document::Position pos;
                    if (map_mouse_to_position(mouse.x, mouse.y, pos)) {
                        const auto now = std::chrono::steady_clock::now();
                        const bool same_position = (last_click_position_.page == pos.page &&
                                                    last_click_position_.row == pos.row &&
                                                    last_click_position_.col == pos.col);
                        const bool within_double_click = same_position &&
                                                         (now - last_click_time_) < std::chrono::milliseconds(400);

                        if (!within_double_click) {
                            click_count_ = 1;
                        } else {
                            ++click_count_;
                            if (click_count_ > 3) {
                                click_count_ = 3;
                            }
                        }

                        last_click_position_ = pos;
                        last_click_time_ = now;

                        if (click_count_ == 1) {
                            is_selecting_ = true;
                            editor_->document().clear_selection();
                            editor_->document().set_selection(pos, pos);
                            editor_->document().set_cursor(pos.page, pos.row, pos.col);
                        } else if (click_count_ == 2) {
                            is_selecting_ = false;
                            const auto bounds = editor_->document().word_selection_bounds(pos);
                            editor_->document().set_selection(bounds.first, bounds.second);
                            editor_->document().set_cursor(pos.page, pos.row, pos.col);
                        } else {
                            is_selecting_ = false;
                            const auto bounds = editor_->document().paragraph_selection_bounds(pos);
                            editor_->document().set_selection(bounds.first, bounds.second);
                            editor_->document().set_cursor(pos.page, pos.row, pos.col);
                        }

                        ensure_cursor_visible();
                        screen.PostEvent(Event::Custom);
                        return true;
                    }
                } else if (mouse.motion == Mouse::Moved) {
                    if (is_selecting_) {
                        sord::editor::Document::Position pos;
                        if (map_mouse_to_position(mouse.x, mouse.y, pos)) {
                            auto start = editor_->document().selection_start();
                            editor_->document().set_selection(start, pos);
                            editor_->document().set_cursor(pos.page, pos.row, pos.col);
                            ensure_cursor_visible();
                            screen.PostEvent(Event::Custom);
                            return true;
                        }
                    }
                } else if (mouse.motion == Mouse::Released) {
                    if (is_selecting_) {
                        is_selecting_ = false;
                        auto start = editor_->document().selection_start();
                        auto end = editor_->document().selection_end();
                        if (start == end) {
                            editor_->document().clear_selection();
                        }
                        screen.PostEvent(Event::Custom);
                        return true;
                    }
                }
            } else if (mouse.button == Mouse::Right && mouse.motion == Mouse::Pressed) {
                sord::editor::Document::Position pos;
                if (map_mouse_to_position(mouse.x, mouse.y, pos)) {
                    show_context_menu_ = true;
                    context_menu_x_ = mouse.x;
                    context_menu_y_ = mouse.y;
                    screen.PostEvent(Event::Custom);
                    return true;
                }
            } else if (mouse.button == Mouse::WheelUp) {
                scroll_view(-1);
                screen.PostEvent(Event::Custom);
                return true;
            } else if (mouse.button == Mouse::WheelDown) {
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

bool Application::map_absolute_line_to_page_row(std::size_t absolute_line, std::size_t& page_out, std::size_t& row_out) const {
    const auto& pages = editor_->document().pages();
    std::size_t current_absolute = 0;
    for (std::size_t page_idx = 0; page_idx < pages.size(); ++page_idx) {
        if (page_idx != 0) {
            if (current_absolute == absolute_line) {
                return false;
            }
            current_absolute += 1;
        }
        std::size_t num_lines = pages[page_idx].lines().size();
        if (absolute_line >= current_absolute && absolute_line < current_absolute + num_lines) {
            page_out = page_idx;
            row_out = absolute_line - current_absolute;
            return true;
        }
        current_absolute += num_lines;
    }
    return false;
}

bool Application::map_mouse_to_position(int mx, int my, sord::editor::Document::Position& pos_out) {
    int content_x_min = editor_box_.x_min + 1;
    int content_x_max = editor_box_.x_max - 1;
    int content_y_min = editor_box_.y_min + 1;
    int content_y_max = editor_box_.y_max - 1;

    if (mx < content_x_min || mx > content_x_max || my < content_y_min || my > content_y_max) {
        return false;
    }

    int relative_y = my - content_y_min;
    std::size_t absolute_line = editor_scroll_offset_ + relative_y;

    std::size_t page_idx = 0;
    std::size_t row_idx = 0;
    if (!map_absolute_line_to_page_row(absolute_line, page_idx, row_idx)) {
        return false;
    }

    const auto& pages = editor_->document().pages();
    const auto& line_str = pages[page_idx].lines()[row_idx];

    int relative_x = mx - content_x_min;
    std::size_t col_idx = std::min(static_cast<std::size_t>(std::max(0, relative_x)), line_str.size());

    pos_out = sord::editor::Document::Position{page_idx, row_idx, col_idx};
    return true;
}

void Application::copy_selection_to_clipboard() {
    auto& doc = editor_->document();
    if (!doc.has_selection()) return;

    auto start = doc.selection_start();
    auto end = doc.selection_end();
    if (end < start) {
        std::swap(start, end);
    }

    std::string text_str;
    const auto& pages = doc.pages();
    for (std::size_t p = start.page; p <= end.page; ++p) {
        if (p >= pages.size()) break;
        const auto& lines = pages[p].lines();
        std::size_t start_r = (p == start.page) ? start.row : 0;
        std::size_t end_r = (p == end.page) ? end.row : (lines.empty() ? 0 : lines.size() - 1);

        for (std::size_t r = start_r; r <= end_r; ++r) {
            if (r >= lines.size()) break;
            const auto& line = lines[r];
            std::size_t start_c = (p == start.page && r == start.row) ? start.col : 0;
            std::size_t end_c = (p == end.page && r == end.row) ? end.col : line.size();

            if (start_c > line.size()) start_c = line.size();
            if (end_c > line.size()) end_c = line.size();

            if (p > start.page || r > start_r) {
                text_str += "\n";
            }
            text_str += line.substr(start_c, end_c - start_c);
        }
    }

    internal_clipboard_ = text_str;

    // Try system clipboard tools using popen
    FILE* pipe = popen("wl-copy", "w");
    if (pipe) {
        std::fwrite(text_str.data(), 1, text_str.size(), pipe);
        pclose(pipe);
    } else {
        pipe = popen("xclip -selection clipboard", "w");
        if (pipe) {
            std::fwrite(text_str.data(), 1, text_str.size(), pipe);
            pclose(pipe);
        } else {
            pipe = popen("xsel --clipboard --input", "w");
            if (pipe) {
                std::fwrite(text_str.data(), 1, text_str.size(), pipe);
                pclose(pipe);
            }
        }
    }
}

void Application::paste_from_clipboard() {
    std::string text_str;
    bool system_read = false;

    FILE* pipe = popen("wl-paste -n 2>/dev/null", "r");
    if (!pipe) {
        pipe = popen("xclip -selection clipboard -o 2>/dev/null", "r");
    }
    if (!pipe) {
        pipe = popen("xsel --clipboard --output 2>/dev/null", "r");
    }

    if (pipe) {
        char buffer[256];
        while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            text_str += buffer;
        }
        pclose(pipe);
        if (!text_str.empty()) {
            system_read = true;
        }
    }

    if (!system_read) {
        text_str = internal_clipboard_;
    }

    if (!text_str.empty()) {
        editor_->document().insert_text(text_str);
        mark_modified();
        ensure_cursor_visible();
    }
}

void Application::cut_selection_to_clipboard() {
    if (editor_->document().has_selection()) {
        copy_selection_to_clipboard();
        editor_->document().delete_selection();
        mark_modified();
        ensure_cursor_visible();
    }
}

}  // namespace app
}  // namespace sord
