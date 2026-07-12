#include "editor/document.hpp"

#include <algorithm>
#include <utility>

namespace sord {
namespace editor {

Document::Document(std::string title) : title_(std::move(title)) {
    lines_.emplace_back();
}

void Document::insert_char(char ch) {
    if (ch == '\n') {
        insert_newline();
        return;
    }

    if (cursor_row_ >= lines_.size()) {
        lines_.emplace_back();
    }

    auto& line = lines_[cursor_row_];
    if (cursor_column_ > line.size()) {
        cursor_column_ = line.size();
    }
    line.insert(line.begin() + static_cast<std::ptrdiff_t>(cursor_column_), ch);
    ++cursor_column_;
}

void Document::insert_newline() {
    if (cursor_row_ >= lines_.size()) {
        lines_.emplace_back();
    }

    auto& current = lines_[cursor_row_];
    if (cursor_column_ > current.size()) {
        cursor_column_ = current.size();
    }

    std::string rest = current.substr(cursor_column_);
    current.erase(cursor_column_);
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(cursor_row_ + 1), std::move(rest));
    cursor_row_ += 1;
    cursor_column_ = 0;
}

void Document::backspace() {
    if (cursor_row_ == 0 && cursor_column_ == 0) {
        return;
    }

    if (cursor_column_ == 0) {
        if (cursor_row_ == 0) {
            return;
        }
        auto& previous = lines_[cursor_row_ - 1];
        previous += lines_[cursor_row_];
        lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(cursor_row_));
        --cursor_row_;
        cursor_column_ = previous.size();
        return;
    }

    auto& line = lines_[cursor_row_];
    line.erase(line.begin() + static_cast<std::ptrdiff_t>(cursor_column_ - 1));
    --cursor_column_;
}

void Document::move_cursor(int row_delta, int col_delta) {
    long long new_row = static_cast<long long>(cursor_row_) + row_delta;
    long long new_col = static_cast<long long>(cursor_column_) + col_delta;

    new_row = std::clamp(new_row, 0LL, static_cast<long long>(lines_.size() - 1));
    if (new_row != static_cast<long long>(cursor_row_)) {
        cursor_row_ = static_cast<std::size_t>(new_row);
        cursor_column_ = std::min(static_cast<std::size_t>(new_col), lines_[cursor_row_].size());
        return;
    }

    new_col = std::clamp(new_col, 0LL, static_cast<long long>(lines_[cursor_row_].size()));
    cursor_column_ = static_cast<std::size_t>(new_col);
}

const std::vector<std::string>& Document::lines() const {
    return lines_;
}

std::size_t Document::cursor_row() const {
    return cursor_row_;
}

std::size_t Document::cursor_column() const {
    return cursor_column_;
}

const std::string& Document::title() const {
    return title_;
}

void Document::set_title(std::string title) {
    title_ = std::move(title);
}

void Document::normalize_cursor() {
    if (cursor_row_ >= lines_.size()) {
        cursor_row_ = lines_.size() - 1;
    }
    if (cursor_column_ > lines_[cursor_row_].size()) {
        cursor_column_ = lines_[cursor_row_].size();
    }
}

}  // namespace editor
}  // namespace sord
