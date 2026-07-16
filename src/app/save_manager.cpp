#include "app/save_manager.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>

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

// Helper function to escape JSON strings
static std::string escape_json_string(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                        << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

// Helper function to unescape JSON strings
static std::string unescape_json_string(const std::string& str) {
    std::string result;
    for (std::size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case '/': result += '/'; ++i; break;
                case 'b': result += '\b'; ++i; break;
                case 'f': result += '\f'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += str[i];
            }
        } else {
            result += str[i];
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

    // Write JSON header and document info
    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"title\": \"" << escape_json_string(document.title()) << "\",\n";
    out << "  \"pages\": [\n";

    const auto& pages = document.pages();
    for (std::size_t page_idx = 0; page_idx < pages.size(); ++page_idx) {
        const auto& page = pages[page_idx];
        out << "    {\n";
        out << "      \"lines\": [\n";

        const auto& runs = page.runs();
        for (std::size_t line_idx = 0; line_idx < runs.size(); ++line_idx) {
            const auto& line_runs = runs[line_idx];
            out << "        [\n";

            for (std::size_t run_idx = 0; run_idx < line_runs.size(); ++run_idx) {
                const auto& run = line_runs[run_idx];
                out << "          {\n";
                out << "            \"text\": \"" << escape_json_string(run.text) << "\",\n";
                out << "            \"style\": {\n";
                out << "              \"font_family\": \"" << escape_json_string(run.style.font_family) << "\"\n";
                out << "            }\n";
                out << "          }";
                if (run_idx + 1 < line_runs.size()) {
                    out << ",";
                }
                out << "\n";
            }

            out << "        ]";
            if (line_idx + 1 < runs.size()) {
                out << ",";
            }
            out << "\n";
        }

        out << "      ]\n";
        out << "    }";
        if (page_idx + 1 < pages.size()) {
            out << ",";
        }
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    if (!out) {
        error_message = "Failed while writing file.";
        return false;
    }

    return true;
}

bool SaveManager::load(editor::Document& document,
                       const std::filesystem::path& path,
                       std::string& error_message) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        error_message = "Unable to open file for reading.";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // Simple JSON parsing - look for pages and reconstruct document
    // For now, we'll load it as plain text if it doesn't have JSON structure
    if (content.find("\"version\"") != std::string::npos && content.find("\"pages\"") != std::string::npos) {
        // It's a new JSON format - parse it
        std::vector<std::vector<editor::TextRun>> all_pages;
        
        // Find the pages array
        std::size_t pages_start = content.find("\"pages\": [");
        if (pages_start == std::string::npos) {
            error_message = "Invalid JSON format: no pages array found.";
            return false;
        }

        // For simplicity in this implementation, we'll extract text and reconstruct
        // A full JSON parser would be needed for complete fidelity
        // For now, just load the text content
        std::vector<std::string> lines;
        std::istringstream iss(content);
        std::string line;
        bool in_text = false;
        std::string current_text;

        while (std::getline(iss, line)) {
            // Simple extraction of text values from JSON
            std::size_t text_pos = line.find("\"text\": \"");
            if (text_pos != std::string::npos) {
                std::size_t start = text_pos + 9;
                std::size_t end = line.find("\"", start);
                if (end != std::string::npos) {
                    std::string text = line.substr(start, end - start);
                    text = unescape_json_string(text);
                    current_text += text;
                }
            }
            
            // Line ends indicated by empty bracket array end
            if (line.find("        ]") != std::string::npos && !current_text.empty()) {
                lines.emplace_back(std::move(current_text));
                current_text.clear();
            }
        }

        if (!current_text.empty()) {
            lines.emplace_back(std::move(current_text));
        }

        if (lines.empty()) {
            lines.emplace_back();
        }

        document.set_lines(std::move(lines));
        return true;
    } else {
        // Legacy plain text format
        std::vector<std::string> lines;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            lines.emplace_back(line);
        }
        if (lines.empty()) {
            lines.emplace_back();
        }
        document.set_lines(std::move(lines));
        return true;
    }
}

}  // namespace app
}  // namespace sord
