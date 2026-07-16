#include "exporter/pdf_exporter.hpp"
#include "editor/document.hpp"
#include "editor/font_family.hpp"
#include "editor/text_run.hpp"

#include <hpdf.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <map>
#include <string>
#include <vector>

namespace sord {
namespace exporter {

struct PDFExporter::Impl {
    HPDF_Doc  pdf           = nullptr;
    HPDF_Page page          = nullptr;
    double    page_width    = 0;
    double    page_height   = 0;
    double    font_size     = 12.0;
    double    top_margin    = 72.0;   // 1 inch
    double    bottom_margin = 72.0;
    double    left_margin   = 72.0;
    double    right_margin  = 72.0;
    // Cache of resolved PDF built-in fonts (pdf_name -> HPDF_Font)
    std::map<std::string, HPDF_Font> font_cache;
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

// ---------------------------------------------------------------------------
// Map a Sord font family name to the closest available PDF built-in font.
// libharu supports only the 14 standard PDF fonts without embedding;
// we map gracefully so the stored font metadata is never discarded.
// ---------------------------------------------------------------------------
static const char* map_to_pdf_font(const std::string& font_family) {
    // Serif fonts -> Times-Roman
    static const std::vector<std::string> serif_fonts = {
        "Times New Roman", "Georgia", "Garamond", "Palatino Linotype",
        "Book Antiqua", "Bookman Old Style", "Cambria", "Constantia",
        "Baskerville", "Bodoni MT", "Perpetua", "Bell MT",
        "DejaVu Serif", "Liberation Serif", "Noto Serif", "Source Serif Pro",
        "Lucida Bright", "High Tower Text", "Poor Richard", "Bell MT",
        "Footlight MT Light", "Onyx", "Playbill"
    };
    for (const auto& f : serif_fonts) {
        if (font_family == f) return "Times-Roman";
    }

    // Monospace fonts -> Courier
    static const std::vector<std::string> mono_fonts = {
        "Courier New", "Consolas", "Lucida Console", "Ubuntu Mono",
        "DejaVu Sans Mono", "Liberation Mono", "Noto Sans Mono",
        "Source Code Pro", "Roboto Mono", "OCR A", "OCR B"
    };
    for (const auto& f : mono_fonts) {
        if (font_family == f) return "Courier";
    }

    // Symbol-like
    if (font_family == "Segoe UI Symbol") return "Symbol";

    // Everything else (Arial, Calibri, Helvetica, Ubuntu, etc.) -> Helvetica
    return "Helvetica";
}

bool PDFExporter::CreateDocument(std::string& error) {
    impl_->pdf = HPDF_New(HPDF_error_handler, nullptr);
    if (!impl_->pdf) {
        error = "Failed to create PDF object.";
        return false;
    }
    HPDF_SetCompressionMode(impl_->pdf, HPDF_COMP_ALL);
    // Pre-load fallback font
    impl_->font_cache["Helvetica"] = HPDF_GetFont(impl_->pdf, "Helvetica", nullptr);
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
    impl_->page_width  = HPDF_Page_GetWidth(impl_->page);
    impl_->page_height = HPDF_Page_GetHeight(impl_->page);
    return true;
}


// Retrieve (or load) a PDF font handle for the given document font family.
// Substitution is local to PDF export; the .sord file retains the original font_family.
static HPDF_Font resolve_pdf_font(HPDF_Doc pdf, std::map<std::string, HPDF_Font>& font_cache, const std::string& font_family) {
    const char* pdf_name = map_to_pdf_font(font_family);
    auto it = font_cache.find(pdf_name);
    if (it != font_cache.end()) {
        return it->second;
    }
    HPDF_Font f = HPDF_GetFont(pdf, pdf_name, nullptr);
    if (!f) {
        f = font_cache["Helvetica"]; // ultimate fallback
    }
    font_cache[pdf_name] = f;
    return f;
}

// ---------------------------------------------------------------------------
// WriteParagraphRuns — render one line of styled TextRuns
// ---------------------------------------------------------------------------
bool PDFExporter::WriteParagraphRuns(
        const std::vector<editor::TextRun>& line_runs,
        double left, double /*top*/, double right, double bottom,
        double& cursor_y, std::string& error) {

    if (!impl_->page) {
        error = "Page not initialized.";
        return false;
    }

    double leading = impl_->font_size * 1.2;

    // Build a flat list of words with their associated PDF font.
    struct Word { std::string text; HPDF_Font font; };
    std::vector<Word> words;

    for (const auto& run : line_runs) {
        HPDF_Font f = resolve_pdf_font(impl_->pdf, impl_->font_cache, run.style.font_family);
        std::string w;
        for (char c : run.text) {
            if (c == ' ' || c == '\t') {
                if (!w.empty()) { words.push_back({std::move(w), f}); w.clear(); }
            } else {
                w.push_back(c);
            }
        }
        if (!w.empty()) { words.push_back({std::move(w), f}); }
    }

    if (words.empty()) {
        // Blank paragraph: add a small vertical gap
        cursor_y -= impl_->font_size * 0.8;
        return true;
    }

    // Word-wrap using the first word's font for width measurement
    double available_width = right - left;
    std::vector<std::vector<Word>> wrapped_lines;
    std::vector<Word> current_line;

    auto measure_combined = [&](const std::vector<Word>& wl) -> double {
        std::string s;
        for (std::size_t i = 0; i < wl.size(); ++i) {
            if (i) s += ' ';
            s += wl[i].text;
        }
        HPDF_Page_SetFontAndSize(impl_->page, wl[0].font, impl_->font_size);
        return HPDF_Page_TextWidth(impl_->page, s.c_str());
    };

    for (auto& word : words) {
        std::vector<Word> trial = current_line;
        trial.push_back(word);
        double w = measure_combined(trial);
        if (w <= available_width || current_line.empty()) {
            current_line.push_back(std::move(word));
        } else {
            wrapped_lines.push_back(std::move(current_line));
            current_line.clear();
            current_line.push_back(std::move(word));
        }
    }
    if (!current_line.empty()) wrapped_lines.push_back(std::move(current_line));

    // Render each wrapped line word-by-word, advancing x per word
    for (const auto& wline : wrapped_lines) {
        if (cursor_y - leading < bottom) {
            return true; // out of vertical space; stop silently
        }
        double x        = left;
        double baseline = cursor_y - impl_->font_size;
        bool   first    = true;
        for (const auto& word : wline) {
            std::string token = first ? word.text : (" " + word.text);
            first = false;
            HPDF_Page_BeginText(impl_->page);
            HPDF_Page_SetFontAndSize(impl_->page, word.font, impl_->font_size);
            HPDF_Page_TextOut(impl_->page, x, baseline, token.c_str());
            HPDF_Page_EndText(impl_->page);
            // Measure rendered token to advance x
            HPDF_Page_SetFontAndSize(impl_->page, word.font, impl_->font_size);
            x += HPDF_Page_TextWidth(impl_->page, token.c_str());
        }
        cursor_y -= leading;
    }

    return true;
}

// Legacy single-string entry point (satisfies existing API signature)
bool PDFExporter::WriteParagraph(const std::string& text, double left, double top, double right, double bottom, double& cursor_y, std::string& error) {
    editor::TextRun run;
    run.text = text;
    // style.font_family defaults to "Arial" (see style.hpp)
    const std::vector<editor::TextRun> runs = {run};
    return WriteParagraphRuns(runs, left, top, right, bottom, cursor_y, error);
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

bool PDFExporter::Export(const sord::editor::Document& doc,
                         const std::filesystem::path& out_path,
                         std::string& error) {
    if (!CreateDocument(error)) return false;

    const auto& pages = doc.pages();
    for (const auto& p : pages) {
        if (!CreatePage(error)) return false;

        double left    = impl_->left_margin;
        double right   = impl_->page_width - impl_->right_margin;
        double top     = impl_->page_height - impl_->top_margin;
        double bottom  = impl_->bottom_margin;
        double cursor_y = top;

        // Iterate over runs per paragraph — reading stored font_family from each run.
        // The document font metadata is NEVER discarded; PDF substitution is local only.
        for (const auto& line_runs : p.runs()) {
            if (!WriteParagraphRuns(line_runs, left, top, right, bottom, cursor_y, error)) {
                break;
            }
            if (cursor_y < bottom) break;
        }
    }

    if (!Save(out_path, error)) return false;
    return true;
}

}  // namespace exporter
}  // namespace sord
