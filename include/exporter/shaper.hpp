#pragma once

#include "exporter/font_cache.hpp"
#include "exporter/glyph_run.hpp"

#include <string>

namespace sord {
namespace exporter {

class Shaper {
public:
    // Shape `text` using the given font resource and return glyph positions,
    // advances, offsets, and clusters.
    // The method does not perform drawing or interact with PDF APIs.
    static bool ShapeText(FontResource* font_resource,
                          const std::string& text,
                          double font_size,
                          GlyphRun& out_glyphs);
};

} // namespace exporter
} // namespace sord
