#include <cassert>
#include <iostream>
#include <memory>
#include "editor/editor.hpp"
#include "renderer/editor_renderer.hpp"

int main() {
    auto editor = std::make_shared<sord::editor::Editor>();
    sord::renderer::EditorRenderer renderer(editor);

    // Initial state: cursor is at Ln 1, Col 1. Not full.
    std::string status = renderer.render_status_bar();
    assert(status.find("Ln 1, Col 1") != std::string::npos);
    assert(status.find("Page Full") == std::string::npos);
    // Ensure no raw ANSI escape codes exist
    assert(status.find("\033[") == std::string::npos);

    // Move cursor or insert newlines until page limit is reached
    auto& doc = editor->document();
    std::size_t limit = doc.page_line_limit();
    for (std::size_t i = 1; i < limit; ++i) {
        doc.insert_newline();
    }

    // Now page is full
    status = renderer.render_status_bar();
    assert(status.find("[Page Full - Please add a new page]") != std::string::npos);
    // Ensure there are no ANSI escape codes in the plain text output
    assert(status.find("\033[") == std::string::npos);

    std::cout << "test_editor_renderer passed!" << std::endl;
    return 0;
}
