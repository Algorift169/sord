#include "exporter/pdf_exporter.hpp"
#include "editor/document.hpp"

#include <string>

namespace sord {
namespace exporter {

struct PDFExporter::Impl {};

PDFExporter::PDFExporter() : impl_(new Impl()) {}
PDFExporter::~PDFExporter() { delete impl_; }

bool PDFExporter::CreateDocument(std::string& error) {
    error = "libharu not available in build; PDF export disabled.";
    return false;
}
bool PDFExporter::CreatePage(std::string& error) { error = "libharu not available."; return false; }
bool PDFExporter::WriteParagraph(const std::string&, double, double, double, double, double&, std::string& error) { error = "libharu not available."; return false; }
bool PDFExporter::Save(const std::filesystem::path&, std::string& error) { error = "libharu not available."; return false; }

bool PDFExporter::Export(const sord::editor::Document&, const std::filesystem::path&, std::string& error) {
    error = "PDF export is not available because libharu was not found when building.";
    return false;
}

}  // namespace exporter
}  // namespace sord
