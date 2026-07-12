#include <cassert>
#include <iostream>
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;
    doc.insert_char('H');
    doc.insert_char('e');
    doc.insert_char('l');
    doc.insert_char('l');
    doc.insert_char('o');

    assert(doc.lines().size() == 1);
    assert(doc.lines()[0] == "Hello");

    // Move cursor to middle of "Hello" (after 'e')
    doc.move_cursor(0, -3);
    assert(doc.cursor_column() == 2);

    // Insert newline
    doc.insert_newline();
    assert(doc.lines().size() == 2);
    assert(doc.lines()[0] == "He");
    assert(doc.lines()[1] == "llo");
    assert(doc.cursor_row() == 1);
    assert(doc.cursor_column() == 0);

    // Insert newline at start of line 1
    doc.insert_newline();
    assert(doc.lines().size() == 3);
    assert(doc.lines()[0] == "He");
    assert(doc.lines()[1] == "");
    assert(doc.lines()[2] == "llo");
    assert(doc.cursor_row() == 2);
    assert(doc.cursor_column() == 0);

    std::cout << "test_document_insert_newline passed!" << std::endl;
    return 0;
}
