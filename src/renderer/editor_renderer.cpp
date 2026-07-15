#include "renderer/editor_renderer.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <utility>

using namespace ftxui;

namespace sord {
namespace renderer {

EditorRenderer::EditorRenderer(std::shared_ptr<sord::editor::Editor> editor)
    : editor_(std::move(editor)), page_renderer_(sord::layout::PageLayout(sord::layout::PageLayout::A4_WIDTH,
                                                                          sord::layout::PageLayout::A4_HEIGHT, 2, 1)) {}

std::string EditorRenderer::render_title_bar() const {
    std::ostringstream oss;
    oss << "Sord";
    oss << "  " << editor_->document().title();
    oss << "  " << "[Ready]";
    return oss.str();
}

std::string EditorRenderer::render_toolbar() const {
    return //"Toolbar Titlebar Placeholder\n"
           "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
           //"Sub Toolbar Placeholder\n"
           "\n"
           "\n"
           "\n";
}

std::string EditorRenderer::render_status_bar() const {
    std::ostringstream oss;
    const auto& doc = editor_->document();
    const auto current_page = doc.current_page();
    const auto cursor_row = doc.cursor_row();
    const auto page_limit = doc.page_line_limit();
    
    oss << "Ln " << (cursor_row + 1)
        << ", Col " << (doc.cursor_column() + 1)
        << "  Page " << (current_page + 1);
    
    // Check if current page is at capacity
    if (cursor_row + 1 >= page_limit) {
        oss << "  [Page Full - Please add a new page]";
    }
    
    oss << "                    UTF-8        INSERT";
    return oss.str();
}

static std::string CreatePageSeparator(std::size_t width) {
    return std::string(width, '_');
}

std::string EditorRenderer::render_content() const {
    std::ostringstream oss;
    const auto& pages = editor_->document().pages();
    for (std::size_t page_index = 0; page_index < pages.size(); ++page_index) {
        if (page_index != 0) {
            oss << '\n' << CreatePageSeparator(80) << '\n';
        }
        for (std::size_t i = 0; i < pages[page_index].lines().size(); ++i) {
            if (i != 0) {
                oss << '\n';
            }
            oss << pages[page_index].lines()[i];
        }
    }
    return oss.str();
}

std::vector<Element> EditorRenderer::render_visible_lines(std::size_t width, std::size_t height, std::size_t offset) const {
    std::vector<Element> all_lines;
    const auto& pages = editor_->document().pages();
    const auto current_page = editor_->document().current_page();
    const auto cursor_row = editor_->document().cursor_row();
    const auto cursor_col = editor_->document().cursor_column();

    const auto& doc = editor_->document();
    const auto selection_start = doc.selection_start();
    const auto selection_end = doc.selection_end();

    for (std::size_t page_index = 0; page_index < pages.size(); ++page_index) {
        if (page_index != 0) {
            all_lines.push_back(text(CreatePageSeparator(width)));
        }

        auto page_lines = page_renderer_.render(pages[page_index], width, std::numeric_limits<std::size_t>::max(),
                                               page_index == current_page ? cursor_row : std::size_t(-1),
                                               cursor_col);
        for (std::size_t row = 0; row < page_lines.size(); ++row) {
            auto line = text(page_lines[row]);
            if (doc.has_selection() && page_index == current_page) {
                auto start = selection_start;
                auto end = selection_end;
                if (end < start) {
                    std::swap(start, end);
                }

                if (page_index == start.page && page_index == end.page) {
                    if (row >= start.row && row <= end.row) {
                        if (row == start.row && row == end.row) {
                            auto before = page_lines[row].substr(0, start.col);
                            auto selected = page_lines[row].substr(start.col, end.col - start.col);
                            auto after = page_lines[row].substr(end.col);
                            line = hbox({text(before), text(selected) | inverted, text(after)});
                        } else if (row == start.row) {
                            auto before = page_lines[row].substr(0, start.col);
                            auto selected = page_lines[row].substr(start.col);
                            line = hbox({text(before), text(selected) | inverted});
                        } else if (row == end.row) {
                            auto selected = page_lines[row].substr(0, end.col);
                            auto after = page_lines[row].substr(end.col);
                            line = hbox({text(selected) | inverted, text(after)});
                        } else {
                            line = text(page_lines[row]) | inverted;
                        }
                    }
                } else if (page_index == start.page) {
                    if (row == start.row) {
                        auto before = page_lines[row].substr(0, start.col);
                        auto selected = page_lines[row].substr(start.col);
                        line = hbox({text(before), text(selected) | inverted});
                    }
                } else if (page_index == end.page) {
                    if (row == end.row) {
                        auto selected = page_lines[row].substr(0, end.col);
                        auto after = page_lines[row].substr(end.col);
                        line = hbox({text(selected) | inverted, text(after)});
                    } else if (row < end.row) {
                        line = text(page_lines[row]) | inverted;
                    }
                } else if (page_index > start.page && page_index < end.page) {
                    line = text(page_lines[row]) | inverted;
                }
            }
            all_lines.push_back(line);
        }
    }

    if (all_lines.empty()) {
        all_lines.push_back(text(""));
    }

    std::vector<Element> visible;
    std::size_t start = std::min(offset, all_lines.size());
    for (std::size_t i = start; i < std::min(all_lines.size(), start + height); ++i) {
        visible.push_back(all_lines[i]);
    }
    if (visible.empty()) {
        visible.push_back(text(""));
    }
    return visible;
}

}  // namespace renderer
}  // namespace sord
