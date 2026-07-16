#include <cassert>
#include <iostream>
#include "editor/page_manager.hpp"

int main() {
    sord::editor::PageManager pm;
    pm.page(0).set_lines({"Page 1"});

    pm.add_page();
    assert(pm.size() == 2);
    pm.page(1).set_lines({"Page 2"});

    pm.add_page(0); // Insert at index 0
    assert(pm.size() == 3);
    assert(pm.page(0).lines().size() == 1 && pm.page(0).lines()[0].empty());
    assert(pm.page(1).lines()[0] == "Page 1");

    pm.remove_page(1); // Remove the middle page (Page 1)
    assert(pm.size() == 2);
    assert(pm.page(1).lines()[0] == "Page 2");

    // Remove all pages, should recreate at least one empty page
    pm.remove_page(0);
    pm.remove_page(0);
    assert(pm.size() == 1);
    assert(pm.page(0).lines().size() == 1 && pm.page(0).lines()[0].empty());

    std::cout << "test_page_manager_add_remove passed!" << std::endl;
    return 0;
}
