#pragma once

#include <memory>
#include <string>
#include <vector>

#include "editor/editor.hpp"
#include "renderer/page_renderer.hpp"

namespace sord {
namespace renderer {

class EditorRenderer {
public:
    explicit EditorRenderer(std::shared_ptr<sord::editor::Editor> editor);

    [[nodiscard]] std::string render_title_bar() const;
    [[nodiscard]] std::string render_toolbar() const;
    [[nodiscard]] std::string render_status_bar() const;
    [[nodiscard]] std::string render_content() const;
    [[nodiscard]] std::vector<std::string> render_visible_lines(std::size_t width = 80,
                                                               std::size_t height = 24,
                                                               std::size_t offset = 0) const;

private:
    std::shared_ptr<sord::editor::Editor> editor_;
    PageRenderer page_renderer_;
};

}  // namespace renderer
}  // namespace sord
