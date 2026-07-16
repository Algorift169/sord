#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "page_manager.hpp"
#include "../layout/page_layout.hpp"
#include "font_family.hpp"

namespace sord {
namespace editor {

class Document {
public:
    struct Position {
        std::size_t page = 0;
        std::size_t row = 0;
        std::size_t col = 0;

        bool operator==(const Position& other) const {
            return page == other.page && row == other.row && col == other.col;
        }

        bool operator<(const Position& other) const {
            if (page != other.page) return page < other.page;
            if (row != other.row) return row < other.row;
            return col < other.col;
        }

        bool operator<=(const Position& other) const {
            return *this < other || *this == other;
        }

        bool operator>(const Position& other) const {
            return !(*this <= other);
        }

        bool operator>=(const Position& other) const {
            return !(*this < other);
        }
    };

    Document(std::string title = "Untitled.sord");

    void insert_char(char ch);
    void insert_newline();
    void backspace();
    void move_cursor(int row_delta, int col_delta);
    void insert_text(const std::string& text);

    [[nodiscard]] const std::vector<std::string>& lines() const;
    [[nodiscard]] std::vector<Page>& pages();
    [[nodiscard]] const std::vector<Page>& pages() const;
    [[nodiscard]] std::size_t page_count() const;
    [[nodiscard]] std::size_t current_page() const;
    void add_page();
    [[nodiscard]] std::size_t cursor_row() const;
    [[nodiscard]] std::size_t cursor_column() const;
    [[nodiscard]] const std::string& title() const;
    [[nodiscard]] std::size_t page_line_limit() const;

    void set_title(std::string title);
    void set_lines(std::vector<std::string> lines);
    // Load rich document content from serialized run data (preserves per-run font_family).
    // Each outer vector is a page; each middle vector is a paragraph/line;
    // each inner vector contains the styled runs of that paragraph.
    void set_pages_from_runs(
        std::vector<std::vector<std::vector<TextRun>>> pages_runs);
    void set_cursor(std::size_t page, std::size_t row, std::size_t col);
    
    // Selection methods
    void set_selection(const Position& start, const Position& end);
    [[nodiscard]] Position selection_start() const;
    [[nodiscard]] Position selection_end() const;
    [[nodiscard]] bool has_selection() const;
    void clear_selection();
    void delete_selection();
    void select_all();
    void undo();
    void redo();
    [[nodiscard]] std::pair<Position, Position> word_selection_bounds(const Position& pos) const;
    [[nodiscard]] std::pair<Position, Position> paragraph_selection_bounds(const Position& pos) const;

    // Font family methods
    [[nodiscard]] const std::string& current_font_family() const;
    void set_current_font_family(const std::string& font);

private:
    void normalize_cursor();
    void snapshot_history();

    std::string title_;
    PageManager page_manager_;
    std::size_t current_page_ = 0;
    std::size_t cursor_row_ = 0;
    std::size_t cursor_column_ = 0;
    Position selection_start_;
    Position selection_end_;
    bool has_selection_ = false;
    using PageRuns = std::vector<std::vector<TextRun>>;
    using HistoryEntry = std::vector<PageRuns>;

    std::vector<HistoryEntry> history_;
    std::size_t history_index_ = 0;
    std::string current_font_family_ = FontFamily::default_font();
    static constexpr std::size_t PAGE_LINE_LIMIT = sord::layout::PageLayout::A4_HEIGHT;
};

}  // namespace editor
}  // namespace sord
