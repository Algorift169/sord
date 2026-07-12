#include "layout/page_layout.hpp"

namespace sord {
namespace layout {

PageLayout::PageLayout(std::size_t width, std::size_t height, std::size_t margin_left, std::size_t margin_top)
    : width_(width), height_(height), margin_left_(margin_left), margin_top_(margin_top) {}

std::size_t PageLayout::width() const {
    return width_;
}

std::size_t PageLayout::height() const {
    return height_;
}

std::size_t PageLayout::margin_left() const {
    return margin_left_;
}

std::size_t PageLayout::margin_top() const {
    return margin_top_;
}

std::size_t PageLayout::content_width() const {
    return width_ > margin_left_ * 2 ? width_ - margin_left_ * 2 : 0;
}

std::size_t PageLayout::content_height() const {
    return height_ > margin_top_ * 2 ? height_ - margin_top_ * 2 : 0;
}

}  // namespace layout
}  // namespace sord
