#pragma once

#include <string>
#include "style.hpp"

namespace sord {
namespace editor {

// A text run with associated styling information
struct TextRun {
    std::string text;
    Style style;

    // Default constructor
    TextRun() = default;

    // Convenience constructor
    explicit TextRun(std::string t, const Style& s = Style()) 
        : text(std::move(t)), style(s) {}

    // Convenience constructor with just font
    TextRun(std::string t, std::string font) 
        : text(std::move(t)), style{std::move(font)} {}

    bool operator==(const TextRun& other) const {
        return text == other.text && style == other.style;
    }

    bool operator!=(const TextRun& other) const {
        return !(*this == other);
    }

    // Return the plain text without style information
    [[nodiscard]] const std::string& get_text() const {
        return text;
    }

    // Get the font family
    [[nodiscard]] const std::string& get_font() const {
        return style.font_family;
    }
};

}  // namespace editor
}  // namespace sord
