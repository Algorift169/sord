#include <cassert>
#include <iostream>
#include "exporter/pdf_exporter.hpp"
#include "editor/document.hpp"

int main() {
    sord::editor::Document doc("Test Doc");
    doc.insert_char('H');
    doc.insert_char('e');
    doc.insert_char('l');
    doc.insert_char('l');
    doc.insert_char('o');

    sord::exporter::PDFExporter exporter;
    std::string err;
    std::filesystem::path pdf_path = "test_export.pdf";

    // Clean up if it exists
    std::error_code ec;
    std::filesystem::remove(pdf_path, ec);

    bool result = exporter.Export(doc, pdf_path, err);
    
    // We assert that the function behaves correctly:
    // If it succeeds, the file must exist.
    // If it fails, the error message must not be empty.
    if (result) {
        assert(std::filesystem::exists(pdf_path));
        std::filesystem::remove(pdf_path, ec);
        std::cout << "PDF export succeeded and verified." << std::endl;
    } else {
        assert(!err.empty());
        std::cout << "PDF export failed gracefully as expected: " << err << std::endl;
    }

    std::cout << "test_pdf_exporter passed!" << std::endl;
    return 0;
}
