#pragma once

#include <cstddef>
#include <vector>

#include "editor/page.hpp"

namespace sord {
namespace editor {

class PageManager {
public:
    PageManager();
    explicit PageManager(std::size_t initial_pages);

    [[nodiscard]] std::vector<Page>& pages();
    [[nodiscard]] const std::vector<Page>& pages() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] Page& page(std::size_t index);
    [[nodiscard]] const Page& page(std::size_t index) const;

    void add_page(std::size_t index = static_cast<std::size_t>(-1));
    void remove_page(std::size_t index);
    void reorder_page(std::size_t from, std::size_t to);

private:
    std::vector<Page> pages_;
};

}  // namespace editor
}  // namespace sord
