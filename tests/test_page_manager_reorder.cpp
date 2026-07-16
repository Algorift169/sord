#include <cassert>
#include <iostream>
#include "editor/page_manager.hpp"

int main() {
    sord::editor::PageManager pm;
    pm.page(0).set_lines({"First"});

    pm.add_page();
    pm.page(1).set_lines({"Second"});

    pm.add_page();
    pm.page(2).set_lines({"Third"});

    assert(pm.size() == 3);

    // Reorder: move "First" (0) to end (2)
    pm.reorder_page(0, 2);
    assert(pm.page(0).lines()[0] == "Second");
    assert(pm.page(1).lines()[0] == "Third");
    assert(pm.page(2).lines()[0] == "First");

    // Reorder with invalid indices should do nothing
    pm.reorder_page(0, 5);
    assert(pm.page(0).lines()[0] == "Second");

    std::cout << "test_page_manager_reorder passed!" << std::endl;
    return 0;
}
