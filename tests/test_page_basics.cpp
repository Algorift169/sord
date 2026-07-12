#include <cassert>
#include <iostream>
#include "editor/page.hpp"

int main() {
    sord::editor::Page page;
    assert(page.lines().empty());

    page.append_line("Hello");
    assert(page.lines().size() == 1);
    assert(page.lines()[0] == "Hello");

    page.insert_line(0, "First");
    assert(page.lines().size() == 2);
    assert(page.lines()[0] == "First");
    assert(page.lines()[1] == "Hello");

    page.erase_line(0);
    assert(page.lines().size() == 1);
    assert(page.lines()[0] == "Hello");

    std::vector<std::string> new_lines = {"One", "Two", "Three"};
    page.set_lines(new_lines);
    assert(page.lines().size() == 3);
    assert(page.lines()[1] == "Two");

    std::cout << "test_page_basics passed!" << std::endl;
    return 0;
}
