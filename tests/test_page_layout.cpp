#include <cassert>
#include <iostream>
#include "layout/page_layout.hpp"

int main() {
    // Default layout
    sord::layout::PageLayout default_layout;
    assert(default_layout.width() == 80);
    assert(default_layout.height() == 24);
    assert(default_layout.margin_left() == 2);
    assert(default_layout.margin_top() == 1);
    assert(default_layout.content_width() == 76); // 80 - 2 * 2
    assert(default_layout.content_height() == 22); // 24 - 1 * 2

    // Custom layout
    sord::layout::PageLayout custom_layout(100, 50, 5, 2);
    assert(custom_layout.width() == 100);
    assert(custom_layout.height() == 50);
    assert(custom_layout.margin_left() == 5);
    assert(custom_layout.margin_top() == 2);
    assert(custom_layout.content_width() == 90);  // 100 - 2 * 5
    assert(custom_layout.content_height() == 46); // 50 - 2 * 2

    std::cout << "test_page_layout passed!" << std::endl;
    return 0;
}
