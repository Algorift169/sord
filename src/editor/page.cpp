#include "editor/page.hpp"

#include <utility>

namespace sord {
namespace editor {

Page::Page() {
    runs_.emplace_back();
    lines_cache_valid_ = false;
}

Page::Page(std::vector<std::string> lines) {
    set_lines(std::move(lines));
}

void Page::rebuild_lines_cache() const {
    lines_cache_.clear();
    for (const auto& line_runs : runs_) {
        std::string line;
        for (const auto& run : line_runs) {
            line += run.text;
        }
        lines_cache_.emplace_back(std::move(line));
    }
    lines_cache_valid_ = true;
}

std::vector<std::string>& Page::lines() {
    if (!lines_cache_valid_) {
        rebuild_lines_cache();
    }
    return lines_cache_;
}

const std::vector<std::string>& Page::lines() const {
    if (!lines_cache_valid_) {
        rebuild_lines_cache();
    }
    return lines_cache_;
}

const std::vector<std::vector<TextRun>>& Page::runs() const {
    return runs_;
}

std::vector<std::vector<TextRun>>& Page::runs() {
    lines_cache_valid_ = false;
    return runs_;
}

void Page::set_lines(std::vector<std::string> lines) {
    runs_.clear();
    for (const auto& line : lines) {
        std::vector<TextRun> line_runs;
        if (!line.empty()) {
            line_runs.emplace_back(line);
        }
        runs_.emplace_back(std::move(line_runs));
    }
    if (runs_.empty()) {
        runs_.emplace_back();
    }
    lines_cache_valid_ = false;
}

void Page::append_line(std::string line) {
    std::vector<TextRun> line_runs;
    if (!line.empty()) {
        line_runs.emplace_back(std::move(line));
    }
    runs_.emplace_back(std::move(line_runs));
    lines_cache_valid_ = false;
}

void Page::insert_line(std::size_t index, std::string line) {
    if (index > runs_.size()) {
        index = runs_.size();
    }
    std::vector<TextRun> line_runs;
    if (!line.empty()) {
        line_runs.emplace_back(std::move(line));
    }
    runs_.insert(runs_.begin() + static_cast<std::ptrdiff_t>(index), std::move(line_runs));
    lines_cache_valid_ = false;
}

void Page::erase_line(std::size_t index) {
    if (index >= runs_.size()) {
        return;
    }
    runs_.erase(runs_.begin() + static_cast<std::ptrdiff_t>(index));
    lines_cache_valid_ = false;
}

void Page::set_runs(std::vector<std::vector<TextRun>> runs) {
    runs_ = std::move(runs);
    if (runs_.empty()) {
        runs_.emplace_back();
    }
    lines_cache_valid_ = false;
}

void Page::append_runs(std::vector<TextRun> line_runs) {
    runs_.emplace_back(std::move(line_runs));
    lines_cache_valid_ = false;
}

void Page::insert_runs(std::size_t index, std::vector<TextRun> line_runs) {
    if (index > runs_.size()) {
        index = runs_.size();
    }
    runs_.insert(runs_.begin() + static_cast<std::ptrdiff_t>(index), std::move(line_runs));
    lines_cache_valid_ = false;
}

}  // namespace editor
}  // namespace sord
