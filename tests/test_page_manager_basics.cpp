#include <cassert>
#include <iostream>
#include "editor/page_manager.hpp"

int main() {
    sord::editor::PageManager pm;
    assert(pm.size() == 1);
    assert(pm.page(0).lines().size() == 1 && pm.page(0).lines()[0].empty());

    sord::editor::PageManager pm2(5);
    assert(pm2.size() == 5);

    sord::editor::PageManager pm3(0);
    assert(pm3.size() == 1); // should fallback to at least 1 page

    // Out of bounds page access should clamp to last page
    sord::editor::Page& last = pm2.page(10);
    assert(&last == &pm2.page(4));

    std::cout << "test_page_manager_basics passed!" << std::endl;
    return 0;
}
