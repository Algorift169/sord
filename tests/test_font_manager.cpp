#include <cassert>
#include <iostream>
#include <string>
#include "renderer/menu/font_manager.hpp"

int main() {
    const auto& fonts = sord::renderer::menu::FontManager::font_families();
    assert(!fonts.empty());

    const auto fallback = sord::renderer::menu::FontManager::fallback_font();
    assert(!fallback.empty());

    const auto resolved = sord::renderer::menu::FontManager::resolve_font_file(fallback);
    (void)resolved;

    std::cout << "font manager discovered " << fonts.size() << " families" << std::endl;
    return 0;
}
