#pragma once

#include <hpdf.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace sord {
namespace exporter {

struct FontResource {
    std::string family;
    std::string path;
    FT_Face face = nullptr;
    hb_font_t* hb_font = nullptr;
    HPDF_Font pdf_font = nullptr;
};

class FontCache {
public:
    explicit FontCache(HPDF_Doc pdf);
    ~FontCache();

    // Acquire a font resource for the requested document font family.
    // Returns nullptr only when no font file could be loaded and no fallback font is available.
    FontResource* acquire_font(const std::string& family, std::string& error);

    HPDF_Font fallback_pdf_font() const;

private:
    FontResource* load_font_for_family(const std::string& family, std::string& resolved_path, std::string& error);
    FontResource* load_font_from_path(const std::string& family, const std::string& path, std::string& error);
    static std::string resolve_font_path(const std::string& family);
    static std::string resolve_fallback_path();

    HPDF_Doc pdf_ = nullptr;
    FT_Library ft_library_ = nullptr;
    HPDF_Font fallback_font_ = nullptr;
    std::map<std::string, FontResource> font_by_path_;
    std::mutex mutex_;
};

} // namespace exporter
} // namespace sord
