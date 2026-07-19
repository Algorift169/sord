#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "editor/text_run.hpp"

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
    // Internal document/page lifecycle
    bool CreateDocument(std::string& error);
    bool CreatePage(std::string& error);

    // Write one paragraph from a set of styled text runs.
    // Reads run.style.font_family from each run and renders into the PDF.
    bool WriteParagraphRuns(const std::vector<editor::TextRun>& line_runs,
                            double left, double top, double right, double bottom,
                            double& cursor_y, std::string& error);

    // Legacy plain-text paragraph (delegates to WriteParagraphRuns)
    bool WriteParagraph(const std::string& text, double left, double top,
                        double right, double bottom,
                        double& cursor_y, std::string& error);

    bool Save(const std::filesystem::path& out_path, std::string& error);

    // Pimpl — hides libharu types from the public header
    struct Impl;
    Impl* impl_ = nullptr;
};

}  // namespace exporter
}  // namespace sord
