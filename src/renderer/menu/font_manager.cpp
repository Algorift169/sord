#include "renderer/menu/font_manager.hpp"

#include <algorithm>
#include <fontconfig/fontconfig.h>
#include <set>
#include <utility>

namespace sord {
namespace renderer {
namespace menu {
namespace {

std::vector<std::string> discover_font_families() {
    std::vector<std::string> families;
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config) {
        return families;
    }

    FcPattern* pat = FcPatternCreate();
    if (!pat) {
        FcConfigDestroy(config);
        return families;
    }

    FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, nullptr);
    if (!os) {
        FcPatternDestroy(pat);
        FcConfigDestroy(config);
        return families;
    }

    FcFontSet* fonts = FcFontList(config, pat, os);
    if (fonts) {
        std::set<std::string> unique;
        for (int i = 0; i < fonts->nfont; ++i) {
            FcPattern* font = fonts->fonts[i];
            FcChar8* family = nullptr;
            if (FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch && family) {
                std::string name(reinterpret_cast<const char*>(family));
                if (!name.empty()) {
                    unique.insert(name);
                }
            }
        }
        families.assign(unique.begin(), unique.end());
        std::sort(families.begin(), families.end());
        FcFontSetDestroy(fonts);
    }

    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
    FcConfigDestroy(config);
    return families;
}

std::string discover_fallback_font(const std::vector<std::string>& families) {
    static const std::vector<std::string> preferred = {
        "Noto Sans", "DejaVu Sans", "Liberation Sans", "Arial", "Ubuntu", "Helvetica"
    };
    for (const auto& candidate : preferred) {
        if (std::find(families.begin(), families.end(), candidate) != families.end()) {
            return candidate;
        }
    }
    return families.empty() ? "Arial" : families.front();
}

}  // namespace

std::vector<std::string>& FontManager::mutable_font_families() {
    static std::vector<std::string> families = discover_font_families();
    return families;
}

const std::vector<std::string>& FontManager::font_families() {
    return mutable_font_families();
}

std::string FontManager::fallback_font() {
    return discover_fallback_font(font_families());
}

std::string FontManager::resolve_font_file(const std::string& family) {
    if (family.empty()) {
        return {};
    }

    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config) {
        return {};
    }

    FcPattern* pat = FcNameParse(reinterpret_cast<const FcChar8*>(family.c_str()));
    if (!pat) {
        FcConfigDestroy(config);
        return {};
    }

    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result = FcResultNoMatch;
    FcPattern* match = FcFontMatch(config, pat, &result);
    FcPatternDestroy(pat);
    if (!match) {
        FcConfigDestroy(config);
        return {};
    }

    FcChar8* file = nullptr;
    if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file) {
        std::string path(reinterpret_cast<const char*>(file));
        FcPatternDestroy(match);
        FcConfigDestroy(config);
        return path;
    }

    FcPatternDestroy(match);
    FcConfigDestroy(config);
    return {};
}

bool FontManager::is_available(const std::string& family) {
    return std::find(font_families().begin(), font_families().end(), family) != font_families().end();
}

void FontManager::refresh() {
    mutable_font_families() = discover_font_families();
}

}  // namespace menu
}  // namespace renderer
}  // namespace sord
