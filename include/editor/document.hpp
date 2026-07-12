#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "editor/page_manager.hpp"

namespace sord {
namespace editor {

class Document {
public:
    Document(std::string title = "Untitled.sord");

    void insert_char(char ch);
    void insert_newline();
    void backspace();
    void move_cursor(int row_delta, int col_delta);

    [[nodiscard]] const std::vector<std::string>& lines() const;
    [[nodiscard]] std::vector<Page>& pages();
    [[nodiscard]] const std::vector<Page>& pages() const;
    [[nodiscard]] std::size_t page_count() const;
    [[nodiscard]] std::size_t current_page() const;
    void add_page();
    [[nodiscard]] std::size_t cursor_row() const;
    [[nodiscard]] std::size_t cursor_column() const;
    [[nodiscard]] const std::string& title() const;

    void set_title(std::string title);

private:
    void normalize_cursor();

    std::string title_;
    PageManager page_manager_;
    std::size_t current_page_ = 0;
    std::size_t cursor_row_ = 0;
    std::size_t cursor_column_ = 0;
};

}  // namespace editor
}  // namespace sord
