#pragma once

#include <array>
#include <string_view>

namespace sord {
namespace editor {

// Comprehensive list of supported font families
// Designed to match Microsoft Word and common system fonts
class FontFamily {
public:
    static constexpr std::array<std::string_view, 138> AVAILABLE_FONTS = {
        // Microsoft Office fonts
        "Arial",
        "Arial Black",
        "Arial Narrow",
        "Arial Rounded MT Bold",
        "Calibri",
        "Calibri Light",
        "Cambria",
        "Candara",
        "Century Gothic",
        "Comic Sans MS",
        "Consolas",
        "Courier",
        "Constantia",
        "Corbel",
        "Courier New",
        "Franklin Gothic Medium",
        "Gabriola",
        "Garamond",
        "Georgia",
        "Impact",
        "Lucida Bright",
        "Lucida Console",
        "Lucida Sans",
        "Lucida Sans Unicode",
        "Microsoft Sans Serif",
        "Palatino Linotype",
        "Segoe UI",
        "Segoe UI Light",
        "Segoe UI Semibold",
        "Segoe UI Symbol",
        "Tahoma",
        "Times New Roman",
        "Trebuchet MS",
        "Verdana",

        // Additional system fonts - Windows
        "Bahnschrift",
        "Book Antiqua",
        "Bookman Old Style",
        "Calisto MT",
        "Cambria Math",
        "Candara Light",
        "Century",
        "Didot",
        "Futura",
        "Gill Sans",

        // Additional system fonts - Cross-platform
        "Helvetica",
        "Helvetica Neue",
        "Optima",
        "Rockwell",

        // Additional system fonts - Classical/Serif
        "Baskerville",
        "Bodoni MT",
        "Perpetua",
        "Bell MT",

        // Additional system fonts - Display/Special
        "Berlin Sans FB",
        "Britannic Bold",
        "Copperplate Gothic",
        "Elephant",
        "Engravers MT",
        "Eras Medium ITC",
        "Footlight MT Light",
        "Gigi",
        "Haettenschweiler",
        "High Tower Text",
        "Imprint MT Shadow",
        "Kristen ITC",
        "Magneto",
        "Mistral",
        "Modern No. 20",
        "Monotype Corsiva",
        "Niagara Engraved",
        "OCR A",
        "OCR B",
        "Old English Text MT",
        "Onyx",
        "Playbill",
        "Poor Richard",
        "Rage Italic",
        "Script MT Bold",
        "Showcard Gothic",
        "Snap ITC",
        "Stencil",
        "Tw Cen MT",

        // Linux/Open source fonts
        "Ubuntu",
        "Ubuntu Mono",
        "Ubuntu Condensed",
        "DejaVu Sans",
        "DejaVu Serif",
        "DejaVu Sans Mono",
        "Liberation Sans",
        "Liberation Serif",
        "Liberation Mono",
        "Noto Sans",
        "Noto Serif",
        "Noto Sans Mono",

        // Google and modern fonts
        "Source Sans Pro",
        "Source Serif Pro",
        "Source Code Pro",
        "Roboto",
        "Roboto Mono",
        "Open Sans",
        "Lato",
        "Inter"
    };

    // Get the default font
    static constexpr std::string_view DEFAULT_FONT_VIEW = "Arial";
    
    static std::string default_font() {
        return "Arial";
    }

    // Check if a font name is valid
    static bool is_valid(std::string_view font_name) {
        for (const auto& font : AVAILABLE_FONTS) {
            if (font == font_name) {
                return true;
            }
        }
        return false;
    }

    // Get the count of available fonts
    static constexpr std::size_t count() {
        return AVAILABLE_FONTS.size();
    }
};

}  // namespace editor
}  // namespace sord
