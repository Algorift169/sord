#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../editor/page.hpp"
#include "../layout/page_layout.hpp"

namespace sord {
namespace renderer {

class PageRenderer {
public:
    PageRenderer();
    explicit PageRenderer(sord::layout::PageLayout layout);

    [[nodiscard]] std::vector<std::string> render(const sord::editor::Page& page, std::size_t width = 0,
                                                 std::size_t height = 0,
                                                 std::size_t cursor_row = 0,
                                                 std::size_t cursor_column = 0) const;

private:
    sord::layout::PageLayout layout_;
};

}  // namespace renderer
}  // namespace sord
