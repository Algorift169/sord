#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace sord {
namespace editor {

class Page {
public:
    Page();
    explicit Page(std::vector<std::string> lines);

    [[nodiscard]] std::vector<std::string>& lines();
    [[nodiscard]] const std::vector<std::string>& lines() const;

    void set_lines(std::vector<std::string> lines);
    void append_line(std::string line);
    void insert_line(std::size_t index, std::string line);
    void erase_line(std::size_t index);

private:
    std::vector<std::string> lines_;
};

}  // namespace editor
}  // namespace sord
