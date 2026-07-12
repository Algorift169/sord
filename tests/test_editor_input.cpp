#include <cassert>
#include <iostream>
#include "editor/editor.hpp"

int main() {
    sord::editor::Editor editor("test_doc");
    assert(editor.document().title() == "test_doc");

    editor.handle_char("Hello");
    assert(editor.document().lines()[0] == "Hello");
    assert(editor.document().cursor_column() == 5);

    // Test backspace (key 127 or 8)
    editor.handle_input(127);
    assert(editor.document().lines()[0] == "Hell");
    assert(editor.document().cursor_column() == 4);

    // Test newline (key 10)
    editor.handle_input(10);
    assert(editor.document().lines().size() == 2);
    assert(editor.document().lines()[0] == "Hell");
    assert(editor.document().lines()[1] == "");

    // Test arrow keys: Move up (259)
    editor.handle_input(259);
    assert(editor.document().cursor_row() == 0);

    // Move left (260)
    editor.handle_input(260);
    assert(editor.document().cursor_column() == 3); // from 4 to 3

    std::cout << "test_editor_input passed!" << std::endl;
    return 0;
}
