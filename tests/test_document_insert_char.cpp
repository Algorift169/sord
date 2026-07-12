#include <cassert>
#include <iostream>
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;
    assert(doc.lines().size() == 1);
    assert(doc.lines()[0] == "");
    assert(doc.cursor_row() == 0);
    assert(doc.cursor_column() == 0);

    doc.insert_char('A');
    assert(doc.lines()[0] == "A");
    assert(doc.cursor_column() == 1);

    doc.insert_char('B');
    assert(doc.lines()[0] == "AB");
    assert(doc.cursor_column() == 2);

    doc.move_cursor(0, -1);
    assert(doc.cursor_column() == 1);

    doc.insert_char('C');
    assert(doc.lines()[0] == "ACB");
    assert(doc.cursor_column() == 2);

    std::cout << "test_document_insert_char passed!" << std::endl;
    return 0;
}
