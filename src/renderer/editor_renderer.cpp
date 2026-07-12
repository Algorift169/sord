#include "renderer/editor_renderer.hpp"

#include <algorithm>
#include <sstream>

namespace sord {
namespace renderer {

EditorRenderer::EditorRenderer(std::shared_ptr<sord::editor::Editor> editor) : editor_(std::move(editor)) {}

std::string EditorRenderer::render_title_bar() const {
    std::ostringstream oss;
    oss << "Sord";
    oss << "  " << editor_->document().title();
    oss << "  " << "[Ready]";
    return oss.str();
}

std::string EditorRenderer::render_toolbar() const {
    return "Toolbar Placeholder";
}

std::string EditorRenderer::render_status_bar() const {
    std::ostringstream oss;
    oss << "Ln " << (editor_->document().cursor_row() + 1)
        << ", Col " << (editor_->document().cursor_column() + 1)
        << "                             UTF-8        INSERT";
    return oss.str();
}

std::string EditorRenderer::render_content() const {
    const auto& lines = editor_->document().lines();
    std::ostringstream oss;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            oss << '\n';
        }
        oss << lines[i];
    }
    return oss.str();
}

std::vector<std::string> EditorRenderer::render_visible_lines(std::size_t width, std::size_t height) const {
    const auto& lines = editor_->document().lines();
    const auto cursor_row = editor_->document().cursor_row();
    const auto cursor_col = editor_->document().cursor_column();

    std::size_t start_row = 0;
    if (lines.size() > height) {
        start_row = std::min(cursor_row, lines.size() - height);
    }

    std::vector<std::string> visible;
    for (std::size_t i = start_row; i < std::min(lines.size(), start_row + height); ++i) {
        std::string line = lines[i];
        if (line.size() > width) {
            std::size_t start_col = 0;
            if (i == cursor_row && cursor_col >= width) {
                start_col = cursor_col - width + 1;
            }
            line = line.substr(start_col, width);
        }

        if (i == cursor_row) {
            if (cursor_col < line.size()) {
                line.insert(cursor_col, "█");
            } else {
                line += "█";
            }
        }

        visible.push_back(line);
    }

    if (visible.empty()) {
        visible.emplace_back();
    }
    return visible;
}

}  // namespace renderer
}  // namespace sord
