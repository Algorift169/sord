#pragma once

#include <string>
#include <string_view>

#include "renderer/menu/font_manager.hpp"

namespace sord {
namespace editor {

class FontFamily {
public:
    static constexpr std::string_view DEFAULT_FONT_VIEW = "Arial";

    static std::string default_font() {
        return sord::renderer::menu::FontManager::fallback_font();
    }

    static bool is_valid(std::string_view font_name) {
        return !font_name.empty() &&
               sord::renderer::menu::FontManager::is_available(std::string(font_name));
    }

    static std::size_t count() {
        return sord::renderer::menu::FontManager::font_families().size();
    }
};

}  // namespace editor
}  // namespace sord
