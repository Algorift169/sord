#include "exporter/pdf_exporter.hpp"
#include "editor/document.hpp"
#include "editor/text_run.hpp"
#include "exporter/font_cache.hpp"
#include "exporter/glyph_run.hpp"
#include "exporter/shaper.hpp"
#include "renderer/menu/font_manager.hpp"

#include <hpdf.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <string_view>

namespace sord {
namespace exporter {

struct PDFExporter::Impl {
    HPDF_Doc  pdf           = nullptr;
    HPDF_Page page          = nullptr;
    double    page_width    = 0;
    double    page_height   = 0;
    double    top_margin    = 72.0;   // 1 inch
    double    bottom_margin = 72.0;
    double    left_margin   = 72.0;
    double    right_margin  = 72.0;
    std::unique_ptr<FontCache> font_cache;
    HPDF_Font fallback_font = nullptr;
};

PDFExporter::PDFExporter() : impl_(new Impl()) {}

PDFExporter::~PDFExporter() {
    if (impl_) {
        if (impl_->pdf) {
            HPDF_Free(impl_->pdf);
            impl_->pdf = nullptr;
        }
        delete impl_;
        impl_ = nullptr;
    }
}

static void HPDF_error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void* user_data) {
    (void)user_data;
    std::cerr << "libHaru error handler: code=" << error_no << " detail=" << detail_no << std::endl;
}

static bool is_whitespace_byte(uint8_t byte) {
    return byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r' || byte == '\f' || byte == '\v';
}

static bool is_break_opportunity(const std::string& text, int byte_index) {
    if (byte_index < 0 || byte_index >= static_cast<int>(text.size())) {
        return false;
    }
    return is_whitespace_byte(static_cast<uint8_t>(text[byte_index]));
}

static double glyph_run_width(const GlyphRun& run) {
    double width = 0.0;
    for (const auto& glyph : run.glyphs) {
        width += glyph.x_advance;
    }
    return width;
}

struct ShapedRun {
    const editor::TextRun* original = nullptr;
    const FontResource* font_resource = nullptr;
    GlyphRun glyph_run;
    double width = 0.0;
};

struct GlyphCluster {
    const ShapedRun* shaped_run = nullptr;
    size_t glyph_index = 0;
    size_t glyph_count = 0;
    size_t byte_start = 0;
    size_t byte_end = 0;
    double width = 0.0;
    double x_offset = 0.0;
    double y_offset = 0.0;
    bool break_opportunity = false;
};

static bool shape_runs(const std::vector<editor::TextRun>& line_runs,
                       FontCache* font_cache,
                       double font_size,
                       std::vector<ShapedRun>& out_shaped_runs,
                       std::string& error) {
    out_shaped_runs.clear();
    for (const auto& run : line_runs) {
        if (run.text.empty()) {
            continue;
        }

        std::string local_error;
        FontResource* resource = nullptr;
        if (font_cache) {
            resource = font_cache->acquire_font(run.style.font_family, local_error);
            if (!resource) {
                error = local_error.empty() ? "Failed to load font for PDF export." : local_error;
            }
        }

        GlyphRun glyph_run;
        if (resource) {
            if (!Shaper::ShapeText(resource, run.text, font_size, glyph_run)) {
                glyph_run.glyphs.clear();
            }
        }

        ShapedRun shaped_run;
        shaped_run.original = &run;
        shaped_run.font_resource = resource;
        shaped_run.glyph_run = std::move(glyph_run);
        shaped_run.width = shaped_run.glyph_run.glyphs.empty() ? 0.0 : glyph_run_width(shaped_run.glyph_run);
        out_shaped_runs.push_back(std::move(shaped_run));
    }
    return true;
}

static bool hpdf_check_status(HPDF_STATUS status, HPDF_Doc pdf, std::string& error, const char* context);
static bool hpdf_check_document(HPDF_Doc pdf, std::string& error, const char* context);

static bool render_text_segment(HPDF_Doc pdf,
                                HPDF_Page page,
                                double x,
                                double baseline,
                                std::string_view text,
                                HPDF_Font font,
                                double font_size,
                                std::string& error) {
    if (!hpdf_check_status(HPDF_Page_BeginText(page), pdf, error, "HPDF_Page_BeginText")) {
        return false;
    }
    if (!hpdf_check_status(HPDF_Page_SetFontAndSize(page, font, font_size), pdf, error, "HPDF_Page_SetFontAndSize")) {
        return false;
    }
    if (!hpdf_check_status(HPDF_Page_SetTextMatrix(page, 1, 0, 0, 1, x, baseline), pdf, error, "HPDF_Page_SetTextMatrix")) {
        return false;
    }
    if (!hpdf_check_status(HPDF_Page_ShowText(page, std::string(text).c_str()), pdf, error, "HPDF_Page_ShowText")) {
        return false;
    }
    if (!hpdf_check_status(HPDF_Page_EndText(page), pdf, error, "HPDF_Page_EndText")) {
        return false;
    }
    return true;
}

static bool render_text_cluster(HPDF_Doc pdf,
                                 HPDF_Page page,
                                 double x,
                                 double baseline,
                                 const GlyphCluster& cluster,
                                 HPDF_Font fallback_font,
                                 std::string& error) {
    const auto& run = *cluster.shaped_run;
    const std::string& text = run.original->text;
    const FontResource* resource = run.font_resource;
    HPDF_Font font = fallback_font;
    if (resource && resource->pdf_font) {
        font = resource->pdf_font;
    }

    size_t begin_byte = cluster.byte_start;
    size_t end_byte = cluster.byte_end;
    if (begin_byte > text.size()) begin_byte = text.size();
    if (end_byte > text.size()) end_byte = text.size();
    if (begin_byte >= end_byte) {
        return true;
    }

    std::string_view cluster_text(text.data() + begin_byte, end_byte - begin_byte);
    return render_text_segment(pdf,
                               page,
                               x + cluster.x_offset,
                               baseline + cluster.y_offset,
                               cluster_text,
                               font,
                               run.glyph_run.font_size > 0.0 ? run.glyph_run.font_size : 12.0,
                               error);
}

static bool hpdf_check_status(HPDF_STATUS status, HPDF_Doc pdf, std::string& error, const char* context) {
    if (status != HPDF_OK) {
        error = std::string(context) + " failed. libHaru error " + std::to_string(HPDF_GetError(pdf)) +
                ", detail " + std::to_string(HPDF_GetErrorDetail(pdf)) + ".";
        return false;
    }
    return true;
}

static bool hpdf_check_document(HPDF_Doc pdf, std::string& error, const char* context) {
    if (HPDF_GetError(pdf) != HPDF_OK) {
        error = std::string(context) + " resulted in an invalid PDF document. libHaru error " +
                std::to_string(HPDF_GetError(pdf)) + ", detail " + std::to_string(HPDF_GetErrorDetail(pdf)) + ".";
        return false;
    }
    return true;
}

bool PDFExporter::CreateDocument(std::string& error) {
    impl_->pdf = HPDF_New(HPDF_error_handler, nullptr);
    if (!impl_->pdf) {
        error = "Failed to create PDF object.";
        return false;
    }
    HPDF_SetCompressionMode(impl_->pdf, HPDF_COMP_ALL);
    if (HPDF_UseUTFEncodings(impl_->pdf) != HPDF_OK) {
        error = "Failed to enable UTF encodings for PDF export.";
        return false;
    }

    impl_->font_cache = std::make_unique<FontCache>(impl_->pdf);
    std::string font_error;
    const std::string fallback_family = sord::renderer::menu::FontManager::fallback_font();
    if (!fallback_family.empty()) {
        FontResource* fallback_resource = impl_->font_cache->acquire_font(fallback_family, font_error);
        if (fallback_resource && fallback_resource->pdf_font) {
            impl_->fallback_font = fallback_resource->pdf_font;
        } else {
            // Reset any internal PDF error caused by font loading attempts so fallback can still work.
            HPDF_ResetError(impl_->pdf);
        }
    }

    if (!impl_->fallback_font) {
        impl_->fallback_font = HPDF_GetFont(impl_->pdf, "Helvetica", "UTF-8");
    }

    if (!impl_->fallback_font) {
        error = "Failed to get fallback font for PDF export.";
        if (!font_error.empty()) {
            error += " " + font_error;
        }
        return false;
    }
    return true;
}

bool PDFExporter::CreatePage(std::string& error) {
    if (!impl_->pdf) {
        error = "PDF document is not initialized.";
        return false;
    }
    impl_->page = HPDF_AddPage(impl_->pdf);
    if (!impl_->page) {
        auto code = HPDF_GetError(impl_->pdf);
        auto detail = HPDF_GetErrorDetail(impl_->pdf);
        error = "Failed to create PDF page. libHaru error " + std::to_string(code) + ", detail " + std::to_string(detail) + ".";
        return false;
    }
    HPDF_Page_SetSize(impl_->page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    impl_->page_width  = HPDF_Page_GetWidth(impl_->page);
    impl_->page_height = HPDF_Page_GetHeight(impl_->page);
    return true;
}

bool PDFExporter::WriteParagraphRuns(
        const std::vector<editor::TextRun>& line_runs,
        double left, double /*top*/, double right, double bottom,
        double& cursor_y, std::string& error) {

    if (!impl_->page) {
        error = "Page not initialized.";
        return false;
    }

    const double font_size = 12.0;
    const double available_width = right - left;
    if (available_width <= 0.0) {
        error = "Invalid layout width.";
        return false;
    }

    std::vector<ShapedRun> shaped_runs;
    if (!shape_runs(line_runs, impl_->font_cache.get(), font_size, shaped_runs, error)) {
        return false;
    }

    if (shaped_runs.empty()) {
        cursor_y -= font_size * 1.2;
        return true;
    }

    std::vector<GlyphCluster> clusters;
    for (const auto& run : shaped_runs) {
        const std::string& text = run.original->text;
        if (run.glyph_run.glyphs.empty()) {
            HPDF_Font font = impl_->fallback_font;
            if (run.font_resource && run.font_resource->pdf_font) {
                font = run.font_resource->pdf_font;
            }
            if (!hpdf_check_status(HPDF_Page_SetFontAndSize(impl_->page, font, font_size), impl_->pdf, error, "HPDF_Page_SetFontAndSize")) {
                return false;
            }

            size_t segment_start = 0;
            while (segment_start < text.size()) {
                size_t segment_end = segment_start;
                while (segment_end < text.size() && !is_whitespace_byte(static_cast<uint8_t>(text[segment_end]))) {
                    segment_end += 1;
                }
                while (segment_end < text.size() && is_whitespace_byte(static_cast<uint8_t>(text[segment_end]))) {
                    segment_end += 1;
                }
                if (segment_end > text.size()) {
                    segment_end = text.size();
                }

                std::string_view segment_text(text.data() + segment_start, segment_end - segment_start);
                double width = HPDF_Page_TextWidth(impl_->page, std::string(segment_text).c_str());
                if (!hpdf_check_document(impl_->pdf, error, "HPDF_Page_TextWidth")) {
                    return false;
                }
                bool break_opportunity = segment_end > segment_start && is_whitespace_byte(static_cast<uint8_t>(segment_text.back()));
                clusters.push_back({&run, 0, 0, segment_start, segment_end, width, 0.0, 0.0, break_opportunity});
                segment_start = segment_end;
            }
            if (clusters.empty()) {
                clusters.push_back({&run, 0, 0, 0, 0, 0.0, 0.0, 0.0, false});
            }
            continue;
        }

        const auto& glyphs = run.glyph_run.glyphs;
        size_t glyph_index = 0;
        while (glyph_index < glyphs.size()) {
            const auto& glyph = glyphs[glyph_index];
            size_t cluster_start = static_cast<size_t>(glyph.cluster);
            if (cluster_start > text.size()) {
                cluster_start = text.size();
            }

            double width = 0.0;
            double x_offset = glyph.x_offset;
            double y_offset = glyph.y_offset;
            size_t start_index = glyph_index;
            size_t next_glyph = glyph_index;
            while (next_glyph < glyphs.size() && static_cast<size_t>(glyphs[next_glyph].cluster) == cluster_start) {
                width += glyphs[next_glyph].x_advance;
                next_glyph += 1;
            }

            size_t next_byte = text.size();
            if (next_glyph < glyphs.size()) {
                next_byte = static_cast<size_t>(glyphs[next_glyph].cluster);
            }
            if (next_byte > text.size()) {
                next_byte = text.size();
            }

            bool break_opportunity = is_break_opportunity(text, cluster_start);
            clusters.push_back({&run, start_index, next_glyph - start_index, cluster_start, next_byte, width, x_offset, y_offset, break_opportunity});
            glyph_index = next_glyph;
        }
    }

    if (clusters.empty()) {
        cursor_y -= font_size * 1.2;
        return true;
    }

    auto compute_line_metrics = [&](const std::vector<GlyphCluster>& line) {
        double ascent = 0.0;
        double descent = 0.0;
        for (const auto& cluster : line) {
            if (!cluster.shaped_run) continue;
            const auto* run = cluster.shaped_run;
            if (run->glyph_run.glyphs.empty()) {
                ascent = std::max(ascent, font_size * 0.8);
                descent = std::max(descent, font_size * 0.2);
            } else {
                ascent = std::max(ascent, run->glyph_run.ascender);
                descent = std::max(descent, std::abs(run->glyph_run.descender));
            }
        }
        if (ascent <= 0.0 && descent <= 0.0) {
            ascent = font_size * 0.8;
            descent = font_size * 0.2;
        }
        return std::make_pair(ascent, descent);
    };

    std::vector<std::vector<GlyphCluster>> wrapped_lines;
    wrapped_lines.emplace_back();
    double current_width = 0.0;
    size_t last_break_index = static_cast<size_t>(-1);

    for (const auto& cluster : clusters) {
        double next_width = current_width + cluster.width;
        bool needs_wrap = next_width > available_width && !wrapped_lines.back().empty();
        if (needs_wrap) {
            if (last_break_index != static_cast<size_t>(-1) && last_break_index + 1 < wrapped_lines.back().size()) {
                size_t break_pos = last_break_index + 1;
                std::vector<GlyphCluster> remainder;
                auto& line = wrapped_lines.back();
                for (size_t j = break_pos; j < line.size(); ++j) {
                    remainder.push_back(line[j]);
                }
                line.erase(line.begin() + break_pos, line.end());
                wrapped_lines.emplace_back(std::move(remainder));
                wrapped_lines.back().push_back(cluster);
                current_width = cluster.width;
            } else {
                wrapped_lines.emplace_back();
                wrapped_lines.back().push_back(cluster);
                current_width = cluster.width;
            }
            last_break_index = static_cast<size_t>(-1);
        } else {
            wrapped_lines.back().push_back(cluster);
            current_width = next_width;
            if (cluster.break_opportunity) {
                last_break_index = wrapped_lines.back().size() - 1;
            }
        }
    }

    for (const auto& line : wrapped_lines) {
        if (line.empty()) continue;
        auto [ascent, descent] = compute_line_metrics(line);
        double line_height = ascent + descent + font_size * 0.2;
        if (cursor_y - line_height < bottom) {
            return true;
        }
        double baseline = cursor_y - ascent;

        if (!hpdf_check_status(HPDF_Page_GSave(impl_->page), impl_->pdf, error, "HPDF_Page_GSave")) {
            return false;
        }
        double x = left;
        for (const auto& cluster : line) {
            if (!cluster.shaped_run) {
                continue;
            }
            if (!render_text_cluster(impl_->pdf, impl_->page, x, baseline, cluster, impl_->fallback_font, error)) {
                return false;
            }
            x += cluster.width;
        }
        if (!hpdf_check_status(HPDF_Page_GRestore(impl_->page), impl_->pdf, error, "HPDF_Page_GRestore")) {
            return false;
        }
        cursor_y -= line_height;
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

    const auto parent = out_path.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            error = "Failed to create output directory: " + ec.message();
            return false;
        }
    }

    HPDF_STATUS ret = HPDF_SaveToFile(impl_->pdf, out_path.string().c_str());
    if (ret != HPDF_OK) {
        error = "Failed to save PDF file. libHaru error " + std::to_string(HPDF_GetError(impl_->pdf)) +
                ", detail " + std::to_string(HPDF_GetErrorDetail(impl_->pdf)) + ".";
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
                return false;
            }
            if (cursor_y < bottom) break;
        }
    }

    if (!Save(out_path, error)) return false;
    return true;
}

}  // namespace exporter
}  // namespace sord
