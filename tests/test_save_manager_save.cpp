#include <cassert>
#include <iostream>
#include <fstream>
#include "app/save_manager.hpp"
#include "editor/document.hpp"
#include "editor/text_run.hpp"

int main() {
    sord::editor::Document doc;
    doc.set_title("Test Document");

    // Reconstruct pages_runs structured data to test multiple font families
    // Page 0:
    //   Paragraph 0:
    //     Run 0: "Hello " with Arial
    //     Run 1: "World" with Times New Roman
    //   Paragraph 1:
    //     Run 0: "Linux" with Ubuntu
    sord::editor::TextRun run1;
    run1.text = "Hello ";
    run1.style.font_family = "Arial";

    sord::editor::TextRun run2;
    run2.text = "World";
    run2.style.font_family = "Times New Roman";

    sord::editor::TextRun run3;
    run3.text = "Linux";
    run3.style.font_family = "Ubuntu";

    std::vector<std::vector<std::vector<sord::editor::TextRun>>> pages_runs = {
        {
            { run1, run2 },
            { run3 }
        }
    };

    doc.set_pages_from_runs(pages_runs);

    std::string err;
    std::filesystem::path test_file = "test_output.sord";
    
    // Clean up if it exists
    std::error_code ec;
    std::filesystem::remove(test_file, ec);

    // Save doc
    bool success = sord::app::SaveManager::save(doc, test_file, err);
    assert(success);
    assert(err.empty());

    // Load into a new Document
    sord::editor::Document loaded_doc;
    success = sord::app::SaveManager::load(loaded_doc, test_file, err);
    assert(success);
    assert(err.empty());

    // Verify loaded structure and fonts
    assert(loaded_doc.pages().size() == 1);
    const auto& loaded_runs = loaded_doc.pages()[0].runs();
    assert(loaded_runs.size() == 2);

    // Paragraph 0
    assert(loaded_runs[0].size() == 2);
    assert(loaded_runs[0][0].text == "Hello ");
    assert(loaded_runs[0][0].style.font_family == "Arial");
    assert(loaded_runs[0][1].text == "World");
    assert(loaded_runs[0][1].style.font_family == "Times New Roman");

    // Paragraph 1
    assert(loaded_runs[1].size() == 1);
    assert(loaded_runs[1][0].text == "Linux");
    assert(loaded_runs[1][0].style.font_family == "Ubuntu");

    // Clean up
    std::filesystem::remove(test_file, ec);

    std::cout << "test_save_manager_save passed!" << std::endl;
    return 0;
}
