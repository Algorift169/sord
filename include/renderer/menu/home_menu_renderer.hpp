#pragma once

#include <string>
#include <vector>

namespace sord {
namespace renderer {
namespace menu {

class HomeMenuRenderer {
public:
    static std::string Render(const std::string& current_font = "Arial");
    
    static std::vector<std::string> GetFontList();
};

}  // namespace menu
}  // namespace renderer
}  // namespace sord
