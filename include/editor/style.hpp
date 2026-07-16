#pragma once

#include <string>

namespace sord {
namespace editor {

// Style information for text runs
// Designed to be extensible for future formatting features
struct Style {
    std::string font_family = "Arial";

    // Future formatting properties will be added here:
    // bool bold = false;
    // bool italic = false;
    // bool underline = false;
    // std::string foreground_color = "";
    // std::string background_color = "";
    // int font_size = 12;
    // bool strikethrough = false;

    bool operator==(const Style& other) const {
        return font_family == other.font_family;
    }

    bool operator!=(const Style& other) const {
        return !(*this == other);
    }
};

}  // namespace editor
}  // namespace sord
