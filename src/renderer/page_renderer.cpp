#include "renderer/page_renderer.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace sord {
namespace renderer {

PageRenderer::PageRenderer() : layout_() {}

PageRenderer::PageRenderer(sord::layout::PageLayout layout) : layout_(std::move(layout)) {}

std::vector<std::string> PageRenderer::render(const sord::editor::Page& page, std::size_t width, std::size_t height,
                                              std::size_t cursor_row, std::size_t cursor_column) const {
    const auto effective_width = width == 0 ? layout_.content_width() : width;
    const auto effective_height = height == 0 ? layout_.content_height() : height;
    const auto& lines = page.lines();

    std::vector<std::string> visible;
    const std::size_t start_row = 0;
    for (std::size_t i = start_row; i < lines.size() && i < effective_height; ++i) {
        std::string line = lines[i];
        if (line.size() > effective_width) {
            line = line.substr(0, effective_width);
        }
        if (i == cursor_row && cursor_column < line.size()) {
            line.insert(cursor_column, "▏");
        } else if (i == cursor_row) {
            line += "▏";
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
