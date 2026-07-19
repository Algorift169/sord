#pragma once

#include <string>
#include <vector>

namespace sord {
namespace renderer {
namespace menu {

class FontManager {
public:
    static const std::vector<std::string>& font_families();
    static std::string fallback_font();
    static std::string resolve_font_file(const std::string& family);
    static bool is_available(const std::string& family);
    static void refresh();

private:
    static std::vector<std::string>& mutable_font_families();
};

}  // namespace menu
}  // namespace renderer
}  // namespace sord
