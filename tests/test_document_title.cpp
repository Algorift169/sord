#include <cassert>
#include <iostream>
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;
    assert(doc.title() == "Untitled.sord");

    doc.set_title("my_document.sord");
    assert(doc.title() == "my_document.sord");

    sord::editor::Document doc2("custom_title.sord");
    assert(doc2.title() == "custom_title.sord");

    std::cout << "test_document_title passed!" << std::endl;
    return 0;
}
