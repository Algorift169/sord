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

    sord::editor::Document styled_doc("styled.sord");
    styled_doc.set_current_font_family("Arial");
    styled_doc.insert_char('A');
    styled_doc.set_current_font_family("Times New Roman");
    styled_doc.insert_char('B');
    styled_doc.set_current_font_family("Courier");
    styled_doc.insert_char('C');

    const auto& styled_runs = styled_doc.pages()[0].runs()[0];
    assert(styled_runs.size() == 3);
    assert(styled_runs[0].text == "A");
    assert(styled_runs[0].style.font_family == "Arial");
    assert(styled_runs[1].text == "B");
    assert(styled_runs[1].style.font_family == "Times New Roman");
    assert(styled_runs[2].text == "C");
    assert(styled_runs[2].style.font_family == "Courier");

    std::cout << "document tests passed\n";
    return 0;
}
