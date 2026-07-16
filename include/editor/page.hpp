#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "text_run.hpp"

namespace sord {
namespace editor {

class Page {
public:
    Page();
    explicit Page(std::vector<std::string> lines);

    // Legacy interface - returns combined text from runs
    [[nodiscard]] std::vector<std::string>& lines();
    [[nodiscard]] const std::vector<std::string>& lines() const;

    // Run-based interface for styled text
    [[nodiscard]] const std::vector<std::vector<TextRun>>& runs() const;
    [[nodiscard]] std::vector<std::vector<TextRun>>& runs();

    void set_lines(std::vector<std::string> lines);
    void append_line(std::string line);
    void insert_line(std::size_t index, std::string line);
    void erase_line(std::size_t index);

    // Run-based operations
    void set_runs(std::vector<std::vector<TextRun>> runs);
    void append_runs(std::vector<TextRun> line_runs);
    void insert_runs(std::size_t index, std::vector<TextRun> line_runs);

private:
    // Internal storage: one vector of runs per line
    std::vector<std::vector<TextRun>> runs_;
    
    // Cache of combined text (for backward compatibility)
    mutable std::vector<std::string> lines_cache_;
    mutable bool lines_cache_valid_ = false;

    // Rebuild the cache from runs
    void rebuild_lines_cache() const;
};

}  // namespace editor
}  // namespace sord
