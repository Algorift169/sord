#include <cassert>
#include <iostream>
#include <fstream>
#include "app/save_manager.hpp"
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc;
    doc.insert_char('L');
    doc.insert_char('i');
    doc.insert_char('n');
    doc.insert_char('e');
    doc.insert_char(' ');
    doc.insert_char('1');
    doc.insert_newline();
    doc.insert_char('L');
    doc.insert_char('i');
    doc.insert_char('n');
    doc.insert_char('e');
    doc.insert_char(' ');
    doc.insert_char('2');

    std::string err;
    std::filesystem::path test_file = "test_output.sord";
    
    // Clean up if it exists
    std::error_code ec;
    std::filesystem::remove(test_file, ec);

    bool success = sord::app::SaveManager::save(doc, test_file, err);
    assert(success);
    assert(err.empty());

    // Verify file content
    std::ifstream in(test_file);
    assert(in.is_open());
    std::string l1, l2;
    std::getline(in, l1);
    std::getline(in, l2);
    assert(l1 == "Line 1");
    assert(l2 == "Line 2");
    in.close();

    // Clean up
    std::filesystem::remove(test_file, ec);

    std::cout << "test_save_manager_save passed!" << std::endl;
    return 0;
}
