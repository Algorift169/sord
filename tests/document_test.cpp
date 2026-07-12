#include <cassert>
#include <iostream>

#include "editor/document.hpp"

int main() {
    sord::editor::Document doc("test.sord");
    doc.insert_char('h');
    doc.insert_char('i');
    doc.insert_newline();
    doc.insert_char('!');

    assert(doc.lines().size() == 2);
    assert(doc.lines()[0] == "hi");
    assert(doc.lines()[1] == "!");
    assert(doc.cursor_row() == 1);
    assert(doc.cursor_column() == 1);
    assert(doc.pages().size() == 1);
    assert(doc.pages()[0].lines().size() == 2);
    assert(doc.pages()[0].lines()[0] == "hi");

    std::cout << "document tests passed\n";
    return 0;
}
