#include "exporter/pdf_exporter.hpp"
#include "editor/document.hpp"

#include <hpdf.h>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace sord {
namespace exporter {

struct PDFExporter::Impl {
    HPDF_Doc pdf = nullptr;
    HPDF_Font font = nullptr;
    HPDF_Page page = nullptr;
    double page_width = 0;
    double page_height = 0;
    double font_size = 12.0;
    double top_margin = 72.0;   // 1 inch
    double bottom_margin = 72.0;
    double left_margin = 72.0;
    double right_margin = 72.0;
};

static void HPDF_error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void* user_data) {
    (void)error_no;
    (void)detail_no;
    (void)user_data;
}

PDFExporter::PDFExporter() : impl_(new Impl()) {}

PDFExporter::~PDFExporter() {
    if (impl_) {
        if (impl_->pdf) {
            HPDF_Free(impl_->pdf);
        }
        delete impl_;
    }
}

bool PDFExporter::CreateDocument(std::string& error) {
    impl_->pdf = HPDF_New(HPDF_error_handler, nullptr);
    if (!impl_->pdf) {
        error = "Failed to create PDF object.";
        return false;
    }
    HPDF_SetCompressionMode(impl_->pdf, HPDF_COMP_ALL);
    impl_->font = HPDF_GetFont(impl_->pdf, "Helvetica", nullptr);
    return true;
}

bool PDFExporter::CreatePage(std::string& error) {
    if (!impl_->pdf) {
        error = "PDF document is not initialized.";
        return false;
    }
    impl_->page = HPDF_AddPage(impl_->pdf);
    if (!impl_->page) {
        error = "Failed to create PDF page.";
        return false;
    }
    HPDF_Page_SetSize(impl_->page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    impl_->page_width = HPDF_Page_GetWidth(impl_->page);
    impl_->page_height = HPDF_Page_GetHeight(impl_->page);
    return true;
}

// Simple word-wrapping using font glyph widths
bool PDFExporter::WriteParagraph(const std::string& text, double left, double top, double right, double bottom, double& cursor_y, std::string& error) {
    if (!impl_->page || !impl_->font) {
        error = "Page or font not initialized.";
        return false;
    }

    double available_width = right - left;
    double leading = impl_->font_size * 1.2;

    // split paragraph into words
    std::vector<std::string> words;
    {
        std::string w;
        for (char c : text) {
            if (c == ' ' || c == '\t') {
                if (!w.empty()) {
                    words.push_back(w);
                    w.clear();
                }
            } else {
                w.push_back(c);
            }
        }
        if (!w.empty()) words.push_back(w);
    }

    std::string line;

    auto flush_line = [&](bool force) {
        if (line.empty() && !force) return true;
        if (cursor_y - leading < bottom) {
            // out of vertical space; stop rendering further lines for this page
            return false;
        }
        HPDF_Page_BeginText(impl_->page);
        HPDF_Page_SetFontAndSize(impl_->page, impl_->font, impl_->font_size);
        HPDF_Page_TextOut(impl_->page, left, cursor_y - impl_->font_size, line.c_str());
        HPDF_Page_EndText(impl_->page);
        cursor_y -= leading;
        line.clear();
        return true;
    };

    HPDF_Page_SetFontAndSize(impl_->page, impl_->font, impl_->font_size);
    for (size_t i = 0; i < words.size(); ++i) {
        const std::string& w = words[i];
        std::string trial = line.empty() ? w : (line + " " + w);

        // measure trial width
        double width = HPDF_Page_TextWidth(impl_->page, trial.c_str());

        if (width <= available_width) {
            line = std::move(trial);
        } else {
            // flush current line and start new
            if (!flush_line(false)) return true; // no more space; stop silently
            line = w;
        }
    }

    // flush last line
    if (!flush_line(true)) return true;

    return true;
}

bool PDFExporter::Save(const std::filesystem::path& out_path, std::string& error) {
    if (!impl_->pdf) {
        error = "PDF document is not initialized.";
        return false;
    }
    HPDF_STATUS ret = HPDF_SaveToFile(impl_->pdf, out_path.string().c_str());
    if (ret != HPDF_OK) {
        error = "Failed to save PDF file.";
        return false;
    }
    return true;
}

bool PDFExporter::Export(const sord::editor::Document& doc, const std::filesystem::path& out_path, std::string& error) {
    if (!CreateDocument(error)) return false;

    const auto& pages = doc.pages();
    for (const auto& p : pages) {
        if (!CreatePage(error)) return false;

        double left = impl_->left_margin;
        double right = impl_->page_width - impl_->right_margin;
        double top = impl_->page_height - impl_->top_margin;
        double bottom = impl_->bottom_margin;
        double cursor_y = top;

        // render every line as a paragraph; maintain order
        for (const auto& line : p.lines()) {
            // blank line -> add vertical space
            if (line.empty()) {
                cursor_y -= impl_->font_size * 0.8;
                if (cursor_y < bottom) break;
                continue;
            }
            if (!WriteParagraph(line, left, top, right, bottom, cursor_y, error)) {
                // stop writing further lines on this page if out of space
                break;
            }
        }
    }

    if (!Save(out_path, error)) {
        return false;
    }

    return true;
}

}  // namespace exporter
}  // namespace sord
