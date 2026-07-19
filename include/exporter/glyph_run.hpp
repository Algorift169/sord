#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sord {
namespace exporter {

struct GlyphPosition {
    int32_t glyph_id = 0;
    int32_t cluster = 0;
    double x_offset = 0.0;
    double y_offset = 0.0;
    double x_advance = 0.0;
    double y_advance = 0.0;
};

struct GlyphRun {
    std::string text;
    std::string font_family;
    double font_size = 0.0;
    double ascender = 0.0;
    double descender = 0.0;
    std::vector<GlyphPosition> glyphs;
};

} // namespace exporter
} // namespace sord
