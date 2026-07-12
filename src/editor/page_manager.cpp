#include "editor/page_manager.hpp"

#include <utility>

namespace sord {
namespace editor {

PageManager::PageManager() {
    pages_.emplace_back();
}

PageManager::PageManager(std::size_t initial_pages) {
    for (std::size_t i = 0; i < initial_pages; ++i) {
        pages_.emplace_back();
    }
    if (pages_.empty()) {
        pages_.emplace_back();
    }
}

std::vector<Page>& PageManager::pages() {
    return pages_;
}

const std::vector<Page>& PageManager::pages() const {
    return pages_;
}

std::size_t PageManager::size() const {
    return pages_.size();
}

Page& PageManager::page(std::size_t index) {
    if (index >= pages_.size()) {
        index = pages_.size() - 1;
    }
    return pages_[index];
}

const Page& PageManager::page(std::size_t index) const {
    if (index >= pages_.size()) {
        index = pages_.size() - 1;
    }
    return pages_[index];
}

void PageManager::add_page(std::size_t index) {
    if (index >= pages_.size()) {
        pages_.emplace_back();
        return;
    }
    pages_.insert(pages_.begin() + static_cast<std::ptrdiff_t>(index), Page{});
}

void PageManager::remove_page(std::size_t index) {
    if (index >= pages_.size()) {
        return;
    }
    pages_.erase(pages_.begin() + static_cast<std::ptrdiff_t>(index));
    if (pages_.empty()) {
        pages_.emplace_back();
    }
}

void PageManager::reorder_page(std::size_t from, std::size_t to) {
    if (from >= pages_.size() || to >= pages_.size()) {
        return;
    }
    if (from == to) {
        return;
    }
    auto page = std::move(pages_[from]);
    pages_.erase(pages_.begin() + static_cast<std::ptrdiff_t>(from));
    pages_.insert(pages_.begin() + static_cast<std::ptrdiff_t>(to), std::move(page));
}

}  // namespace editor
}  // namespace sord
