#include <cassert>
#include <iostream>
#include "app/save_manager.hpp"

int main() {
    auto p1 = sord::app::SaveManager::normalize_path("my_file");
    assert(p1.extension() == ".sord");
    assert(p1.filename() == "my_file.sord");

    auto p2 = sord::app::SaveManager::normalize_path("my_file.sord");
    assert(p2.extension() == ".sord");
    assert(p2.filename() == "my_file.sord");

    auto p3 = sord::app::SaveManager::normalize_path("my_file.txt");
    assert(p3.extension() == ".sord");
    assert(p3.filename() == "my_file.sord");

    auto p4 = sord::app::SaveManager::normalize_path("some/dir/file.foo");
    assert(p4.extension() == ".sord");
    assert(p4.filename() == "file.sord");

    std::cout << "test_save_manager_path passed!" << std::endl;
    return 0;
}
