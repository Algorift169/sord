#include "renderer/menu/home_menu_renderer.hpp"

#include <sstream>

namespace sord {
namespace renderer {
namespace menu {

std::string HomeMenuRenderer::Render() {
    std::ostringstream oss;

    // First row: font selector, size, small controls (approximate layout)
    oss << "[Fronts] [F.Size] [t++] [t--] [Text Color] [Highlight] [Clear] [Align] [Paging] [Search....]" << "\n";
    oss << "\n";

    // Second row: big style buttons (B I U etc) and paragraph group placeholders
    oss <<"                    [Mergine] " ;
    oss << "[B]   [I]   [U]   [S]   [X]2   [X^2]        \t     [Left] [Center] [Right] [Justify]" << "\n";
    oss <<"                              ";   
    oss << "             Font                                             Paragraph\n";
    oss <<"---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    oss << "[Bullets]  [Numbering]  [Decrease Indent]  [Increase Indent]  [Line Spacing]  [Paragraph Spacing]" << "\n";
    

    return oss.str();
}

}  // namespace menu
}  // namespace renderer
}  // namespace sord
