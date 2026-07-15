#include <cassert>
#include <iostream>
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;

    doc.insert_char('A');
    doc.insert_char('B');
    doc.insert_char('C');

    doc.undo();
    assert(doc.lines()[0] == "AB");

    doc.redo();
    assert(doc.lines()[0] == "ABC");

    doc.select_all();
    assert(doc.has_selection());
    assert(doc.selection_start().page == 0);
    assert(doc.selection_start().row == 0);
    assert(doc.selection_start().col == 0);
    assert(doc.selection_end().page == 0);
    assert(doc.selection_end().row == 0);
    assert(doc.selection_end().col == 3);

    std::cout << "test_document_undo_redo passed!" << std::endl;
    return 0;
}
