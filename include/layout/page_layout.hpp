#pragma once

#include <cstddef>

namespace sord {
namespace layout {

class PageLayout {
public:
    // A4 standard terminal dimensions: 80 characters wide, 66 lines tall
    // This represents standard A4 paper (210mm × 297mm) in terminal format
    static constexpr std::size_t A4_WIDTH = 80;
    static constexpr std::size_t A4_HEIGHT = 66;

    PageLayout(std::size_t width = A4_WIDTH, std::size_t height = A4_HEIGHT, std::size_t margin_left = 2,
               std::size_t margin_top = 1);

    [[nodiscard]] std::size_t width() const;
    [[nodiscard]] std::size_t height() const;
    [[nodiscard]] std::size_t margin_left() const;
    [[nodiscard]] std::size_t margin_top() const;
    [[nodiscard]] std::size_t content_width() const;
    [[nodiscard]] std::size_t content_height() const;

private:
    std::size_t width_;
    std::size_t height_;
    std::size_t margin_left_;
    std::size_t margin_top_;
};

}  // namespace layout
}  // namespace sord
