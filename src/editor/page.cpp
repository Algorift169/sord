#include "editor/page.hpp"

#include <utility>

namespace sord {
namespace editor {

Page::Page() : lines_() {}

Page::Page(std::vector<std::string> lines) : lines_(std::move(lines)) {}

std::vector<std::string>& Page::lines() {
    return lines_;
}

const std::vector<std::string>& Page::lines() const {
    return lines_;
}

void Page::set_lines(std::vector<std::string> lines) {
    lines_ = std::move(lines);
    if (lines_.empty()) {
        lines_.emplace_back();
    }
}

void Page::append_line(std::string line) {
    lines_.emplace_back(std::move(line));
}

void Page::insert_line(std::size_t index, std::string line) {
    if (index > lines_.size()) {
        index = lines_.size();
    }
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(index), std::move(line));
}

void Page::erase_line(std::size_t index) {
    if (index >= lines_.size()) {
        return;
    }
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(index));
}

}  // namespace editor
}  // namespace sord
