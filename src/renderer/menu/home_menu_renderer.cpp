#include "renderer/menu/home_menu_renderer.hpp"

#include <sstream>
#include <algorithm>
#include "renderer/menu/font_manager.hpp"

namespace sord {
namespace renderer {
namespace menu {

std::vector<std::string> HomeMenuRenderer::GetFontList() {
    return std::vector<std::string>(renderer::menu::FontManager::font_families().begin(),
                                   renderer::menu::FontManager::font_families().end());
}

std::string HomeMenuRenderer::Render(const std::string& current_font) {
    std::ostringstream oss;

    // First row: font selector, size, small controls
    oss << "[Fonts ▼] [F.Size] [t++] [t--] [Text Color] [Highlight] [Clear] [Align] [Paging] [Search....]" << "\n";
    
    // Display current font selection (truncated if needed)
    std::string font_display = current_font;
    if (font_display.length() > 20) {
        font_display = font_display.substr(0, 17) + "...";
    }
    oss << "           " << font_display << "\n";
    oss << "\n";

    // Second row: big style buttons (B I U etc) and paragraph group placeholders
    oss <<"                    [Mergine] " ;
    oss << "[B]   [I]   [U]   [S]   [X]2   [X^2]        \t     [Left] [Center] [Right] [Justify]" << "\n";
    oss <<"                              ";   
    oss << "             Font                                             Paragraph\n";
    oss <<"---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    oss << "[Bullets]  [Numbering]  [Decrease Indent]  [Increase Indent]  [Line Spacing]  [Paragraph Spacing]" << "\n";

    return oss.str();
}

}  // namespace menu
}  // namespace renderer
}  // namespace sord
