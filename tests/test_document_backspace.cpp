#include <cassert>
#include <iostream>
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;
    
    // Backspace on empty document should do nothing
    doc.backspace();
    assert(doc.lines().size() == 1);
    assert(doc.lines()[0] == "");

    doc.insert_char('X');
    doc.insert_char('Y');
    assert(doc.lines()[0] == "XY");

    // Backspace one character
    doc.backspace();
    assert(doc.lines()[0] == "X");
    assert(doc.cursor_column() == 1);

    // Insert newline, then backspace to join lines
    doc.insert_newline();
    assert(doc.lines().size() == 2);
    assert(doc.lines()[0] == "X");
    assert(doc.lines()[1] == "");
    assert(doc.cursor_row() == 1);
    assert(doc.cursor_column() == 0);

    doc.backspace();
    assert(doc.lines().size() == 1);
    assert(doc.lines()[0] == "X");
    assert(doc.cursor_row() == 0);
    assert(doc.cursor_column() == 1);

    std::cout << "test_document_backspace passed!" << std::endl;
    return 0;
}
