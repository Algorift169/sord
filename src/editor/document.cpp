#include "editor/document.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace sord {
namespace editor {
namespace {

std::size_t line_length(const std::vector<TextRun>& line_runs) {
    std::size_t total = 0;
    for (const auto& run : line_runs) {
        total += run.text.size();
    }
    return total;
}

void ensure_line_exists(std::vector<std::vector<TextRun>>& lines, std::size_t row) {
    if (row >= lines.size()) {
        lines.resize(row + 1);
    }
}

void insert_text_into_line(std::vector<TextRun>& line_runs, std::size_t cursor_column, const std::string& text,
                           const std::string& font_family) {
    if (text.empty()) {
        return;
    }

    const std::size_t line_len = line_length(line_runs);
    if (cursor_column > line_len) {
        cursor_column = line_len;
    }

    std::size_t offset = 0;
    for (std::size_t index = 0; index < line_runs.size(); ++index) {
        auto& run = line_runs[index];
        const std::size_t run_len = run.text.size();
        if (cursor_column == offset + run_len) {
            line_runs.insert(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1),
                             TextRun(text, font_family));
            return;
        }
        if (cursor_column < offset + run_len) {
            if (cursor_column == offset) {
                line_runs.insert(line_runs.begin() + static_cast<std::ptrdiff_t>(index),
                                 TextRun(text, font_family));
                return;
            }

            const std::size_t split_offset = cursor_column - offset;
            auto prefix = run.text.substr(0, split_offset);
            auto suffix = run.text.substr(split_offset);
            run.text = std::move(prefix);
            line_runs.insert(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1),
                             TextRun(text, font_family));
            line_runs.insert(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 2),
                             TextRun(std::move(suffix), run.style));
            return;
        }
        offset += run_len;
    }

    line_runs.emplace_back(text, font_family);
}

void erase_text_from_line(std::vector<TextRun>& line_runs, std::size_t cursor_column) {
    if (line_runs.empty() || cursor_column == 0) {
        return;
    }

    const std::size_t line_len = line_length(line_runs);
    if (cursor_column > line_len) {
        cursor_column = line_len;
    }
    if (cursor_column == 0) {
        return;
    }

    std::size_t offset = 0;
    for (std::size_t index = 0; index < line_runs.size(); ++index) {
        auto& run = line_runs[index];
        const std::size_t run_len = run.text.size();
        if (cursor_column <= offset + run_len) {
            if (cursor_column == offset) {
                if (index > 0) {
                    line_runs.erase(line_runs.begin() + static_cast<std::ptrdiff_t>(index - 1));
                }
                return;
            }
            if (cursor_column == offset + run_len) {
                run.text.erase();
                if (run.text.empty()) {
                    line_runs.erase(line_runs.begin() + static_cast<std::ptrdiff_t>(index));
                }
                return;
            }
            const std::size_t split_offset = cursor_column - offset;
            auto prefix = run.text.substr(0, split_offset - 1);
            auto suffix = run.text.substr(split_offset);
            run.text = std::move(prefix);
            line_runs.insert(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1),
                             TextRun(std::move(suffix), run.style));
            return;
        }
        offset += run_len;
    }
}

std::vector<TextRun> split_line_at_cursor(std::vector<TextRun>& line_runs, std::size_t cursor_column) {
    std::vector<TextRun> remainder;
    if (line_runs.empty()) {
        return remainder;
    }

    const std::size_t line_len = line_length(line_runs);
    if (cursor_column >= line_len) {
        return remainder;
    }

    std::size_t offset = 0;
    for (std::size_t index = 0; index < line_runs.size(); ++index) {
        auto& run = line_runs[index];
        const std::size_t run_len = run.text.size();
        if (cursor_column == offset + run_len) {
            if (index + 1 < line_runs.size()) {
                remainder.assign(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1), line_runs.end());
                line_runs.erase(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1), line_runs.end());
            }
            return remainder;
        }
        if (cursor_column < offset + run_len) {
            const std::size_t split_offset = cursor_column - offset;
            auto suffix = run.text.substr(split_offset);
            auto prefix = run.text.substr(0, split_offset);
            run.text = std::move(prefix);
            if (suffix.empty()) {
                if (index + 1 < line_runs.size()) {
                    remainder.assign(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1), line_runs.end());
                    line_runs.erase(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1), line_runs.end());
                }
            } else {
                remainder.emplace_back(std::move(suffix), run.style);
                if (index + 1 < line_runs.size()) {
                    remainder.insert(remainder.end(), line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1),
                                     line_runs.end());
                }
                line_runs.erase(line_runs.begin() + static_cast<std::ptrdiff_t>(index + 1), line_runs.end());
            }
            return remainder;
        }
        offset += run_len;
    }

    return remainder;
}

} // namespace

Document::Document(std::string title) : title_(std::move(title)) {
    page_manager_.page(0).set_lines({""});
    history_.push_back(HistoryEntry{PageRuns{{}}});
    history_index_ = 0;
}

void Document::snapshot_history() {
    history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(history_index_) + 1, history_.end());

    HistoryEntry snapshot;
    for (const auto& page : page_manager_.pages()) {
        snapshot.push_back(page.runs());
    }

    history_.push_back(std::move(snapshot));
    history_index_ = history_.size() - 1;
}

void Document::insert_char(char ch) {
    if (ch == '\n') {
        insert_newline();
        return;
    }

    auto& current_page = page_manager_.page(current_page_);
    auto& paragraph_lines = current_page.runs();
    ensure_line_exists(paragraph_lines, cursor_row_);

    auto& line_runs = paragraph_lines[cursor_row_];
    const std::string text(1, ch);
    insert_text_into_line(line_runs, cursor_column_, text, current_font_family_);
    ++cursor_column_;
    snapshot_history();
}

void Document::insert_newline() {
    auto& current_page = page_manager_.page(current_page_);
    auto& paragraph_lines = current_page.runs();
    ensure_line_exists(paragraph_lines, cursor_row_);

    auto& current_line = paragraph_lines[cursor_row_];
    const std::size_t line_len = line_length(current_line);
    if (cursor_column_ > line_len) {
        cursor_column_ = line_len;
    }

    if (cursor_row_ + 1 >= PAGE_LINE_LIMIT) {
        return;
    }

    snapshot_history();

    std::vector<TextRun> remainder = split_line_at_cursor(current_line, cursor_column_);
    paragraph_lines.insert(paragraph_lines.begin() + static_cast<std::ptrdiff_t>(cursor_row_ + 1), std::move(remainder));
    cursor_row_ += 1;
    cursor_column_ = 0;
}

void Document::backspace() {
    auto& current_page = page_manager_.page(current_page_);
    auto& paragraph_lines = current_page.runs();
    if (cursor_row_ == 0 && cursor_column_ == 0) {
        return;
    }

    if (cursor_column_ == 0) {
        if (cursor_row_ == 0) {
            return;
        }
        auto& previous = paragraph_lines[cursor_row_ - 1];
        previous.insert(previous.end(), paragraph_lines[cursor_row_].begin(), paragraph_lines[cursor_row_].end());
        paragraph_lines.erase(paragraph_lines.begin() + static_cast<std::ptrdiff_t>(cursor_row_));
        --cursor_row_;
        cursor_column_ = line_length(previous);
        snapshot_history();
        return;
    }

    auto& line_runs = paragraph_lines[cursor_row_];
    const std::size_t line_len = line_length(line_runs);
    if (cursor_column_ > line_len) {
        cursor_column_ = line_len;
    }
    if (cursor_column_ == 0) {
        return;
    }

    --cursor_column_;
    erase_text_from_line(line_runs, cursor_column_);
    snapshot_history();
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

void Document::set_pages_from_runs(
    std::vector<std::vector<std::vector<TextRun>>> pages_runs) {
    page_manager_.pages().clear();
    if (pages_runs.empty()) {
        page_manager_.add_page(0);
        page_manager_.page(0).set_lines({""});
    } else {
        for (std::size_t page_idx = 0; page_idx < pages_runs.size(); ++page_idx) {
            page_manager_.add_page(page_idx);
            page_manager_.page(page_idx).set_runs(std::move(pages_runs[page_idx]));
        }
    }
    current_page_ = 0;
    cursor_row_ = 0;
    cursor_column_ = 0;
    history_.clear();
    HistoryEntry snapshot;
    for (const auto& page : page_manager_.pages()) {
        snapshot.push_back(page.runs());
    }
    history_.push_back(std::move(snapshot));
    history_index_ = 0;
}

void Document::set_cursor(std::size_t page, std::size_t row, std::size_t col) {
    current_page_ = page;
    cursor_row_ = row;
    cursor_column_ = col;
    normalize_cursor(); // Ensure cursor is within valid bounds after setting
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

void Document::select_all() {
    const auto& pages = page_manager_.pages();
    if (pages.empty()) {
        return;
    }
    const auto& lines = pages[0].lines();
    Position start{0, 0, 0};
    Position end{0, lines.empty() ? 0 : lines.size() - 1, lines.empty() ? 0 : lines.back().size()};
    set_selection(start, end);
}

void Document::undo() {
    if (history_index_ == 0) {
        return;
    }
    --history_index_;
    page_manager_.pages().clear();
    if (history_[history_index_].empty()) {
        page_manager_.add_page(0);
        page_manager_.page(0).set_runs({{}});
    } else {
        for (std::size_t page_idx = 0; page_idx < history_[history_index_].size(); ++page_idx) {
            page_manager_.add_page(page_idx);
            page_manager_.page(page_idx).set_runs(history_[history_index_][page_idx]);
        }
    }
    current_page_ = 0;
    cursor_row_ = 0;
    cursor_column_ = 0;
}

void Document::redo() {
    if (history_index_ + 1 >= history_.size()) {
        return;
    }
    ++history_index_;
    page_manager_.pages().clear();
    if (history_[history_index_].empty()) {
        page_manager_.add_page(0);
        page_manager_.page(0).set_runs({{}});
    } else {
        for (std::size_t page_idx = 0; page_idx < history_[history_index_].size(); ++page_idx) {
            page_manager_.add_page(page_idx);
            page_manager_.page(page_idx).set_runs(history_[history_index_][page_idx]);
        }
    }
    current_page_ = 0;
    cursor_row_ = 0;
    cursor_column_ = 0;
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

    snapshot_history();
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

const std::string& Document::current_font_family() const {
    return current_font_family_;
}

void Document::set_current_font_family(const std::string& font) {
    if (FontFamily::is_valid(font)) {
        current_font_family_ = font;
    }
}

}  // namespace editor
}  // namespace sord
