#include <cassert>
#include <iostream>
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;
    doc.insert_char('A');
    doc.insert_char('B');
    doc.insert_char('C');
    doc.insert_newline();
    doc.insert_char('D');
    doc.insert_char('E');

    // Document is:
    // Line 0: ABC (len 3)
    // Line 1: DE (len 2)
    // Cursor is at row 1, col 2

    assert(doc.cursor_row() == 1);
    assert(doc.cursor_column() == 2);

    // Move cursor left
    doc.move_cursor(0, -1);
    assert(doc.cursor_column() == 1);

    // Move cursor right past end of line (should clamp to line size)
    doc.move_cursor(0, 10);
    assert(doc.cursor_column() == 2);

    // Move cursor up to line 0 (should clamp column to line 0 size or keep if within bounds)
    doc.move_cursor(-1, 0);
    assert(doc.cursor_row() == 0);
    assert(doc.cursor_column() == 2);

    // Move right to col 3 (end of "ABC")
    doc.move_cursor(0, 1);
    assert(doc.cursor_column() == 3);

    // Move down to line 1 (col 3 should clamp to line 1 size, which is 2)
    doc.move_cursor(1, 0);
    assert(doc.cursor_row() == 1);
    assert(doc.cursor_column() == 2);

    std::cout << "test_document_move_cursor passed!" << std::endl;
    return 0;
}
