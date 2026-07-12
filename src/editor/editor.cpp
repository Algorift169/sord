#include "editor/editor.hpp"

#include <utility>

namespace sord {
namespace editor {

Editor::Editor(std::string title) : document_(std::move(title)) {}

void Editor::handle_input(int key) {
    switch (key) {
        case 259:  // ArrowUp
            document_.move_cursor(-1, 0);
            break;
        case 258:  // ArrowDown
            document_.move_cursor(1, 0);
            break;
        case 260:  // ArrowLeft
            document_.move_cursor(0, -1);
            break;
        case 261:  // ArrowRight
            document_.move_cursor(0, 1);
            break;
        case 127:
        case 8:
            document_.backspace();
            break;
        case 10:
            document_.insert_newline();
            break;
        default:
            break;
    }
}

void Editor::handle_char(const std::string& text) {
    for (char ch : text) {
        document_.insert_char(ch);
    }
}

Document& Editor::document() {
    return document_;
}

const Document& Editor::document() const {
    return document_;
}

}  // namespace editor
}  // namespace sord
