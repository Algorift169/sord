#pragma once

#include <filesystem>
#include <string>

namespace sord {
namespace editor {
class Document;
}
namespace exporter {

class PDFExporter {
public:
    PDFExporter();
    ~PDFExporter();

    // Export the given document to the provided path. On failure, returns false
    // and sets `error` with a human readable message.
    bool Export(const sord::editor::Document& doc, const std::filesystem::path& out_path, std::string& error);

private:
    // internal helpers matching class responsibility
    bool CreateDocument(std::string& error);
    bool CreatePage(std::string& error);
    bool WriteParagraph(const std::string& text, double left, double top, double right, double bottom, double& cursor_y, std::string& error);
    bool Save(const std::filesystem::path& out_path, std::string& error);

    struct Impl;
    Impl* impl_ = nullptr;
};

}  // namespace exporter
}  // namespace sord
