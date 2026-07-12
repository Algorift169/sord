#pragma once

#include <filesystem>
#include <string>

#include "editor/document.hpp"

namespace sord {
namespace app {

class SaveManager {
public:
    static std::filesystem::path normalize_path(std::string path);
    static bool save(const editor::Document& document,
                     const std::filesystem::path& path,
                     std::string& error_message);
};

}  // namespace app
}  // namespace sord
