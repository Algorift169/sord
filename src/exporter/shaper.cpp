#include "exporter/shaper.hpp"

#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cassert>
#include <cstring>

namespace sord {
namespace exporter {

bool Shaper::ShapeText(FontResource* font_resource,
                       const std::string& text,
                       double font_size,
                       GlyphRun& out_glyphs) {
    if (!font_resource || !font_resource->face || !font_resource->hb_font) {
        return false;
    }
    if (text.empty()) {
        out_glyphs = GlyphRun{};
        return true;
    }

    FT_Face face = font_resource->face;
    if (FT_Set_Char_Size(face, 0, static_cast<FT_F26Dot6>(font_size * 64.0), 0, 72)) {
        return false;
    }

    hb_font_t* hb_font = font_resource->hb_font;
    hb_ft_font_set_funcs(hb_font);
    hb_font_set_scale(hb_font, static_cast<int>(font_size * 64.0), static_cast<int>(font_size * 64.0));

    hb_buffer_t* buffer = hb_buffer_create();
    if (!buffer) return false;

    hb_buffer_add_utf8(buffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(buffer);
    hb_shape(hb_font, buffer, nullptr, 0);

    unsigned int glyph_count = 0;
    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &glyph_count);
    hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &glyph_count);
    if (!infos || !positions) {
        hb_buffer_destroy(buffer);
        return false;
    }

    out_glyphs.text = text;
    out_glyphs.font_family = font_resource->family;
    out_glyphs.font_size = font_size;
    out_glyphs.ascender = static_cast<double>(face->size->metrics.ascender) / 64.0;
    out_glyphs.descender = static_cast<double>(face->size->metrics.descender) / 64.0;
    out_glyphs.glyphs.clear();
    out_glyphs.glyphs.reserve(glyph_count);

    for (unsigned int i = 0; i < glyph_count; ++i) {
        GlyphPosition glyph;
        glyph.glyph_id = static_cast<int32_t>(infos[i].codepoint);
        glyph.cluster = static_cast<int32_t>(infos[i].cluster);
        glyph.x_offset = static_cast<double>(positions[i].x_offset) / 64.0;
        glyph.y_offset = static_cast<double>(positions[i].y_offset) / 64.0;
        glyph.x_advance = static_cast<double>(positions[i].x_advance) / 64.0;
        glyph.y_advance = static_cast<double>(positions[i].y_advance) / 64.0;
        out_glyphs.glyphs.push_back(glyph);
    }

    hb_buffer_destroy(buffer);
    return true;
}

} // namespace exporter
} // namespace sord
