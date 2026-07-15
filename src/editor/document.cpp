#include "editor/document.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace sord {
namespace editor {

Document::Document(std::string title) : title_(std::move(title)) {
    page_manager_.page(0).set_lines({""});
}

void Document::insert_char(char ch) {
    if (ch == '\n') {
        insert_newline();
        return;
    }

    auto& current_page = page_manager_.page(current_page_);
    auto& lines = current_page.lines();
    if (cursor_row_ >= lines.size()) {
        lines.emplace_back();
    }

    auto& line = lines[cursor_row_];
    if (cursor_column_ > line.size()) {
        cursor_column_ = line.size();
    }
    line.insert(line.begin() + static_cast<std::ptrdiff_t>(cursor_column_), ch);
    ++cursor_column_;
}

void Document::insert_newline() {
    auto& current_page = page_manager_.page(current_page_);
    auto& lines = current_page.lines();
    if (cursor_row_ >= lines.size()) {
        lines.emplace_back();
    }

    auto& current = lines[cursor_row_];
    if (cursor_column_ > current.size()) {
        cursor_column_ = current.size();
    }

    // Check if adding a new line would exceed the page limit
    if (cursor_row_ + 1 >= PAGE_LINE_LIMIT) {
        // Don't allow new line - page is at capacity
        return;
    }

    std::string rest = current.substr(cursor_column_);
    current.erase(cursor_column_);
    lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(cursor_row_ + 1), std::move(rest));
    cursor_row_ += 1;
    cursor_column_ = 0;
}

void Document::backspace() {
    auto& current_page = page_manager_.page(current_page_);
    auto& lines = current_page.lines();
    if (cursor_row_ == 0 && cursor_column_ == 0) {
        return;
    }

    if (cursor_column_ == 0) {
        if (cursor_row_ == 0) {
            return;
        }
        auto& previous = lines[cursor_row_ - 1];
        previous += lines[cursor_row_];
        lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(cursor_row_));
        --cursor_row_;
        cursor_column_ = previous.size();
        return;
    }

    auto& line = lines[cursor_row_];
    line.erase(line.begin() + static_cast<std::ptrdiff_t>(cursor_column_ - 1));
    --cursor_column_;
}

void Document::move_cursor(int row_delta, int col_delta) {
    if (row_delta < 0 && cursor_row_ == 0 && current_page_ > 0) {
        --current_page_;
        auto& previous_lines = page_manager_.page(current_page_).lines();
        cursor_row_ = previous_lines.empty() ? 0 : previous_lines.size() - 1;
        cursor_column_ = std::min(cursor_column_, previous_lines[cursor_row_].size());
        return;
    }

    auto& current_page = page_manager_.page(current_page_);
    auto& lines = current_page.lines();
    if (row_delta > 0 && cursor_row_ + 1 >= lines.size() && current_page_ + 1 < page_manager_.size()) {
        ++current_page_;
        cursor_row_ = 0;
        auto& next_lines = page_manager_.page(current_page_).lines();
        cursor_column_ = std::min(cursor_column_, next_lines[cursor_row_].size());
        return;
    }

    long long new_row = static_cast<long long>(cursor_row_) + row_delta;
    long long new_col = static_cast<long long>(cursor_column_) + col_delta;

    new_row = std::clamp(new_row, 0LL, static_cast<long long>(lines.size() - 1));
    if (new_row != static_cast<long long>(cursor_row_)) {
        cursor_row_ = static_cast<std::size_t>(new_row);
        cursor_column_ = std::min(static_cast<std::size_t>(new_col), lines[cursor_row_].size());
        return;
    }

    new_col = std::clamp(new_col, 0LL, static_cast<long long>(lines[cursor_row_].size()));
    cursor_column_ = static_cast<std::size_t>(new_col);
}

const std::vector<std::string>& Document::lines() const {
    return page_manager_.page(current_page_).lines();
}

void Document::add_page() {
    page_manager_.add_page(page_manager_.size());
    current_page_ = page_manager_.size() - 1;
    cursor_row_ = 0;
    cursor_column_ = 0;
}

std::vector<Page>& Document::pages() {
    return page_manager_.pages();
}

const std::vector<Page>& Document::pages() const {
    return page_manager_.pages();
}

std::size_t Document::page_count() const {
    return page_manager_.size();
}

std::size_t Document::current_page() const {
    return current_page_;
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

std::size_t Document::page_line_limit() const {
    return PAGE_LINE_LIMIT;
}

void Document::set_title(std::string title) {
    title_ = std::move(title);
}

void Document::set_lines(std::vector<std::string> lines) {
    page_manager_.pages().clear();
    page_manager_.add_page(0);
    page_manager_.page(0).set_lines(std::move(lines));
    current_page_ = 0;
    cursor_row_ = 0;
    cursor_column_ = 0;
}

void Document::set_cursor(std::size_t page, std::size_t row, std::size_t col) {
    current_page_ = page;
    cursor_row_ = row;
    cursor_column_ = col;
    normalize_cursor();
}

void Document::insert_text(const std::string& text) {
    for (char ch : text) {
        if (ch == '\n') {
            insert_newline();
        } else {
            insert_char(ch);
        }
    }
}

void Document::set_selection(const Position& start, const Position& end) {
    selection_start_ = start;
    selection_end_ = end;
    has_selection_ = (start != end);
}

Document::Position Document::selection_start() const {
    return selection_start_;
}

Document::Position Document::selection_end() const {
    return selection_end_;
}

bool Document::has_selection() const {
    return has_selection_;
}

void Document::clear_selection() {
    has_selection_ = false;
    selection_start_ = {0, 0, 0};
    selection_end_ = {0, 0, 0};
}

void Document::delete_selection() {
    if (!has_selection_) return;

    auto start = selection_start_;
    auto end = selection_end_;
    if (end < start) {
        std::swap(start, end);
    }

    const auto& pages = page_manager_.pages();
    if (start.page >= pages.size()) return;

    if (start.page == end.page) {
        auto& page = page_manager_.page(start.page);
        auto& lines = page.lines();

        if (start.row == end.row) {
            if (start.row < lines.size()) {
                auto& line = lines[start.row];
                if (end.col > line.size()) end.col = line.size();
                line.erase(start.col, end.col - start.col);
            }
        } else {
            if (start.row < lines.size()) {
                auto& start_line = lines[start.row];
                if (end.row < lines.size()) {
                    start_line += lines[end.row].substr(end.col);
                }
            }

            if (end.row < lines.size()) {
                lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(end.row));
            }
            if (start.row + 1 < lines.size()) {
                lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(start.row + 1),
                            lines.begin() + static_cast<std::ptrdiff_t>(end.row));
            }
        }

        current_page_ = start.page;
        cursor_row_ = start.row;
        cursor_column_ = start.col;
        normalize_cursor();
    }

    clear_selection();
}

std::pair<Document::Position, Document::Position> Document::word_selection_bounds(const Position& pos) const {
    const auto& pages = page_manager_.pages();
    if (pos.page >= pages.size()) {
        return {pos, pos};
    }

    const auto& lines = pages[pos.page].lines();
    if (pos.row >= lines.size()) {
        return {pos, pos};
    }

    const auto& line = lines[pos.row];
    std::size_t col = std::min(pos.col, line.size());
    std::size_t start = col;
    std::size_t end = col;

    while (start > 0 && !std::isspace(static_cast<unsigned char>(line[start - 1]))) {
        --start;
    }
    while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end]))) {
        ++end;
    }

    return {{pos.page, pos.row, start}, {pos.page, pos.row, end}};
}

std::pair<Document::Position, Document::Position> Document::paragraph_selection_bounds(const Position& pos) const {
    const auto& pages = page_manager_.pages();
    if (pos.page >= pages.size()) {
        return {pos, pos};
    }

    const auto& lines = pages[pos.page].lines();
    if (lines.empty()) {
        return {pos, pos};
    }

    std::size_t start_row = pos.row;
    while (start_row > 0 && !lines[start_row - 1].empty()) {
        --start_row;
    }

    std::size_t end_row = pos.row;
    while (end_row + 1 < lines.size() && !lines[end_row + 1].empty()) {
        ++end_row;
    }

    Position start{pos.page, start_row, 0};
    Position end{pos.page, end_row, lines[end_row].size()};
    return {start, end};
}

void Document::normalize_cursor() {
    auto& current_page = page_manager_.page(current_page_);
    auto& lines = current_page.lines();
    if (cursor_row_ >= lines.size()) {
        cursor_row_ = lines.size() - 1;
    }
    if (cursor_column_ > lines[cursor_row_].size()) {
        cursor_column_ = lines[cursor_row_].size();
    }
}

}  // namespace editor
}  // namespace sord
