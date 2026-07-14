#include "renderer/editor_renderer.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

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

std::vector<std::string> EditorRenderer::render_visible_lines(std::size_t width, std::size_t height, std::size_t offset) const {
    std::vector<std::string> all_lines;
    const auto& pages = editor_->document().pages();
    const auto current_page = editor_->document().current_page();
    const auto cursor_row = editor_->document().cursor_row();
    const auto cursor_col = editor_->document().cursor_column();

    for (std::size_t page_index = 0; page_index < pages.size(); ++page_index) {
        if (page_index != 0) {
            all_lines.push_back(CreatePageSeparator(width));
        }

        auto page_lines = page_renderer_.render(pages[page_index], width, std::numeric_limits<std::size_t>::max(),
                                               page_index == current_page ? cursor_row : std::size_t(-1),
                                               cursor_col);
        all_lines.insert(all_lines.end(), page_lines.begin(), page_lines.end());
    }

    if (all_lines.empty()) {
        all_lines.emplace_back();
    }

    std::vector<std::string> visible;
    std::size_t start = std::min(offset, all_lines.size());
    for (std::size_t i = start; i < std::min(all_lines.size(), start + height); ++i) {
        visible.push_back(all_lines[i]);
    }
    if (visible.empty()) {
        visible.emplace_back();
    }
    return visible;
}

}  // namespace renderer
}  // namespace sord
