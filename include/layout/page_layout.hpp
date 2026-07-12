#pragma once

#include <cstddef>

namespace sord {
namespace layout {

class PageLayout {
public:
    PageLayout(std::size_t width = 80, std::size_t height = 24, std::size_t margin_left = 2,
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
