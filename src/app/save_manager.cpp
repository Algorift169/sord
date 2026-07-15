#include "app/save_manager.hpp"

#include <fstream>

namespace sord {
namespace app {

std::filesystem::path SaveManager::normalize_path(std::string path) {
    std::filesystem::path result(std::move(path));
    if (result.has_filename()) {
        auto filename = result.filename().string();
        if (filename.find('.') == std::string::npos) {
            result += ".sord";
        }
    }
    return result;
}

bool SaveManager::save(const editor::Document& document,
                       const std::filesystem::path& path,
                       std::string& error_message) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error_message = "Unable to open file for writing.";
        return false;
    }

    for (std::size_t row = 0; row < document.lines().size(); ++row) {
        out << document.lines()[row];
        if (row + 1 < document.lines().size()) {
            out << '\n';
        }
    }

    if (!out) {
        error_message = "Failed while writing file.";
        return false;
    }

    return true;
}

}  // namespace app
}  // namespace sord
