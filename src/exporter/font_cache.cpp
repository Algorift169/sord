#include "exporter/font_cache.hpp"
#include "renderer/menu/font_manager.hpp"

#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cassert>
#include <iostream>

namespace sord {
namespace exporter {

FontCache::FontCache(HPDF_Doc pdf)
    : pdf_(pdf) {
    if (FT_Init_FreeType(&ft_library_)) {
        ft_library_ = nullptr;
    }
}

FontCache::~FontCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : font_by_path_) {
        if (kv.second.hb_font) {
            hb_font_destroy(kv.second.hb_font);
        }
        if (kv.second.face) {
            FT_Done_Face(kv.second.face);
        }
    }
    if (ft_library_) {
        FT_Done_FreeType(ft_library_);
    }
}

FontResource* FontCache::acquire_font(const std::string& family, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string resolved_path;
    if (!family.empty()) {
        auto it = std::find_if(font_by_path_.begin(), font_by_path_.end(),
            [&](auto& kv) { return kv.second.family == family; });
        if (it != font_by_path_.end()) {
            auto& resource = it->second;
            if (!resource.face || !resource.hb_font) {
                error = "Cached font resource is invalid.";
                return nullptr;
            }
            return &resource;
        }
    }

    return load_font_for_family(family, resolved_path, error);
}

FontResource* FontCache::load_font_for_family(const std::string& family, std::string& resolved_path, std::string& error) {
    resolved_path = resolve_font_path(family);
    if (!resolved_path.empty()) {
        FontResource* resource = load_font_from_path(family.empty() ? "" : family, resolved_path, error);
        if (resource) {
            return resource;
        }
    }

    const std::string fallback_path = resolve_fallback_path();
    if (fallback_path.empty()) {
        error = "Unable to resolve any font file for PDF export.";
        return nullptr;
    }

    if (fallback_path != resolved_path) {
        resolved_path = fallback_path;
    }
    return load_font_from_path(family.empty() ? "" : family, resolved_path, error);
}

FontResource* FontCache::load_font_from_path(const std::string& family, const std::string& path, std::string& error) {
    assert(pdf_);
    if (!ft_library_) {
        error = "FreeType failed to initialize.";
        return nullptr;
    }

    auto it = font_by_path_.find(path);
    if (it != font_by_path_.end()) {
        return &it->second;
    }

    FT_Face face = nullptr;
    if (FT_New_Face(ft_library_, path.c_str(), 0, &face)) {
        error = "Failed to load font face from path: " + path;
        return nullptr;
    }

    hb_font_t* hb_font = hb_ft_font_create(face, nullptr);
    if (!hb_font) {
        FT_Done_Face(face);
        error = "Failed to create HarfBuzz font for: " + path;
        return nullptr;
    }

    const char* loaded_name = HPDF_LoadTTFontFromFile(pdf_, path.c_str(), HPDF_TRUE);
    std::cerr << "FontCache: trying load path=" << path << " loaded_name=" << (loaded_name ? loaded_name : "(null)") << "\n";
    if (!loaded_name) {
        hb_font_destroy(hb_font);
        FT_Done_Face(face);
        HPDF_ResetError(pdf_);
        error = "Failed to load font into PDF: " + path;
        return nullptr;
    }

    HPDF_Font pdf_font = HPDF_GetFont(pdf_, loaded_name, "UTF-8");
    std::cerr << "FontCache: HPDF_GetFont result=" << (pdf_font ? "ok" : "null") << " for name=" << (loaded_name ? loaded_name : "(null)") << "\n";
    if (!pdf_font) {
        hb_font_destroy(hb_font);
        FT_Done_Face(face);
        HPDF_ResetError(pdf_);
        error = "Failed to obtain PDF font handle for: " + path;
        return nullptr;
    }

    FontResource resource;
    resource.family = family;
    resource.path = path;
    resource.face = face;
    resource.hb_font = hb_font;
    resource.pdf_font = pdf_font;
    auto [inserted, _] = font_by_path_.emplace(path, std::move(resource));
    return &inserted->second;
}

std::string FontCache::resolve_font_path(const std::string& family) {
    if (!family.empty()) {
        std::string path = sord::renderer::menu::FontManager::resolve_font_file(family);
        if (!path.empty()) return path;
    }
    return {};
}

std::string FontCache::resolve_fallback_path() {
    std::string fallback_family = sord::renderer::menu::FontManager::fallback_font();
    if (fallback_family.empty()) return {};
    return sord::renderer::menu::FontManager::resolve_font_file(fallback_family);
}

HPDF_Font FontCache::fallback_pdf_font() const {
    return fallback_font_;
}

} // namespace exporter
} // namespace sord
