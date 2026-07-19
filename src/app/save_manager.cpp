#include "app/save_manager.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "editor/page.hpp"
#include "editor/text_run.hpp"
#include "editor/font_family.hpp"

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

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------

static std::string escape_json_string(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
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


// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------

bool SaveManager::save(const editor::Document& document,
                       const std::filesystem::path& path,
                       std::string& error_message) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        error_message = "Unable to open file for writing.";
        return false;
    }

    out << "{\n";
    out << "  \"version\": 2,\n";
    out << "  \"title\": \"" << escape_json_string(document.title()) << "\",\n";
    out << "  \"pages\": [\n";

    const auto& pages = document.pages();
    for (std::size_t page_idx = 0; page_idx < pages.size(); ++page_idx) {
        const auto& page = pages[page_idx];
        out << "    {\n";
        out << "      \"paragraphs\": [\n";

        const auto& runs = page.runs();
        for (std::size_t line_idx = 0; line_idx < runs.size(); ++line_idx) {
            const auto& line_runs = runs[line_idx];
            out << "        {\n";
            out << "          \"runs\": [\n";

            for (std::size_t run_idx = 0; run_idx < line_runs.size(); ++run_idx) {
                const auto& run = line_runs[run_idx];
                out << "            {\n";
                out << "              \"text\": \"" << escape_json_string(run.text) << "\",\n";
                out << "              \"style\": {\n";
                out << "                \"font_family\": \"" << escape_json_string(run.style.font_family) << "\"\n";
                out << "              }\n";
                out << "            }";
                if (run_idx + 1 < line_runs.size()) {
                    out << ",";
                }
                out << "\n";
            }

            out << "          ]\n";
            out << "        }";
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

// ---------------------------------------------------------------------------
// Minimal hand-rolled JSON parser  (no external dependencies)
// ---------------------------------------------------------------------------
// We only need to parse the structure we write ourselves, so a targeted
// recursive-descent parser is far simpler than a general-purpose one.

namespace {

struct Parser {
    const std::string& src;
    std::size_t pos = 0;

    // Advance past whitespace
    void skip_ws() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' ||
               src[pos] == '\n' || src[pos] == '\r')) {
            ++pos;
        }
    }

    // Expect a literal character, throw on mismatch
    void expect(char c) {
        skip_ws();
        if (pos >= src.size() || src[pos] != c) {
            throw std::runtime_error(
                std::string("Expected '") + c + "' at position " + std::to_string(pos));
        }
        ++pos;
    }

    // Parse a JSON string and return its (unescaped) value
    std::string parse_string() {
        skip_ws();
        expect('"');
        std::string result;
        while (pos < src.size() && src[pos] != '"') {
            if (src[pos] == '\\' && pos + 1 < src.size()) {
                ++pos; // consume backslash
                switch (src[pos]) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u': {
                        // Basic BMP unicode: read 4 hex digits
                        if (pos + 4 < src.size()) {
                            std::string hex = src.substr(pos + 1, 4);
                            pos += 4;
                            // Convert to UTF-8 (simple ASCII range only for our use case)
                            unsigned int code = 0;
                            for (char h : hex) {
                                code <<= 4;
                                if (h >= '0' && h <= '9') code |= (h - '0');
                                else if (h >= 'a' && h <= 'f') code |= (h - 'a' + 10);
                                else if (h >= 'A' && h <= 'F') code |= (h - 'A' + 10);
                            }
                            if (code < 0x80) {
                                result += static_cast<char>(code);
                            } else if (code < 0x800) {
                                result += static_cast<char>(0xC0 | (code >> 6));
                                result += static_cast<char>(0x80 | (code & 0x3F));
                            } else {
                                result += static_cast<char>(0xE0 | (code >> 12));
                                result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                                result += static_cast<char>(0x80 | (code & 0x3F));
                            }
                        }
                        break;
                    }
                    default: result += src[pos]; break;
                }
                ++pos;
            } else {
                result += src[pos++];
            }
        }
        expect('"');
        return result;
    }

    // Skip over any JSON value without storing it
    void skip_value() {
        skip_ws();
        if (pos >= src.size()) return;
        char c = src[pos];
        if (c == '"') { parse_string(); }
        else if (c == '{') { skip_object(); }
        else if (c == '[') { skip_array();  }
        else {
            // number / true / false / null
            while (pos < src.size() && src[pos] != ',' && src[pos] != '}' && src[pos] != ']') {
                ++pos;
            }
        }
    }

    void skip_object() {
        expect('{');
        skip_ws();
        if (pos < src.size() && src[pos] == '}') { ++pos; return; }
        while (pos < src.size()) {
            parse_string(); // key
            skip_ws();
            expect(':');
            skip_value();
            skip_ws();
            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
            else break;
        }
        expect('}');
    }

    void skip_array() {
        expect('[');
        skip_ws();
        if (pos < src.size() && src[pos] == ']') { ++pos; return; }
        while (pos < src.size()) {
            skip_value();
            skip_ws();
            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
            else break;
        }
        expect(']');
    }

    // Parse a run object: { "text": "...", "style": { "font_family": "..." } }
    editor::TextRun parse_run() {
        editor::TextRun run;
        expect('{');
        skip_ws();
        while (pos < src.size() && src[pos] != '}') {
            std::string key = parse_string();
            skip_ws();
            expect(':');
            skip_ws();
            if (key == "text") {
                run.text = parse_string();
            } else if (key == "style") {
                // Parse style object
                expect('{');
                skip_ws();
                while (pos < src.size() && src[pos] != '}') {
                    std::string style_key = parse_string();
                    skip_ws();
                    expect(':');
                    skip_ws();
                    if (style_key == "font_family") {
                        run.style.font_family = parse_string();
                    } else {
                        skip_value();
                    }
                    skip_ws();
                    if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
                    else break;
                }
                expect('}');
            } else {
                skip_value();
            }
            skip_ws();
            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
            else break;
        }
        expect('}');
        // Preserve the stored font family for round-tripping and future fallback.
        // Missing fonts are handled later during rendering/export, not by mutating the document.
        if (run.style.font_family.empty()) {
            run.style.font_family = editor::FontFamily::default_font();
        }
        return run;
    }

    // Parse a paragraph object: { "runs": [ ... ] }
    std::vector<editor::TextRun> parse_paragraph() {
        std::vector<editor::TextRun> line_runs;
        expect('{');
        skip_ws();
        while (pos < src.size() && src[pos] != '}') {
            std::string key = parse_string();
            skip_ws();
            expect(':');
            skip_ws();
            if (key == "runs") {
                expect('[');
                skip_ws();
                while (pos < src.size() && src[pos] != ']') {
                    line_runs.push_back(parse_run());
                    skip_ws();
                    if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
                    else break;
                }
                expect(']');
            } else {
                skip_value();
            }
            skip_ws();
            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
            else break;
        }
        expect('}');
        return line_runs;
    }

    // Parse a page object: { "paragraphs": [ ... ] }
    std::vector<std::vector<editor::TextRun>> parse_page() {
        std::vector<std::vector<editor::TextRun>> page_runs;
        expect('{');
        skip_ws();
        while (pos < src.size() && src[pos] != '}') {
            std::string key = parse_string();
            skip_ws();
            expect(':');
            skip_ws();
            // Support both "paragraphs" (v2) and legacy "lines" (v1 name)
            if (key == "paragraphs" || key == "lines") {
                expect('[');
                skip_ws();
                while (pos < src.size() && src[pos] != ']') {
                    // Each element could be a paragraph object { "runs": [...] }
                    // or (legacy v1) an array of run objects [ {...}, ... ]
                    if (src[pos] == '{') {
                        page_runs.push_back(parse_paragraph());
                    } else if (src[pos] == '[') {
                        // legacy v1 format: array of run objects
                        std::vector<editor::TextRun> line_runs;
                        expect('[');
                        skip_ws();
                        while (pos < src.size() && src[pos] != ']') {
                            line_runs.push_back(parse_run());
                            skip_ws();
                            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
                            else break;
                        }
                        expect(']');
                        page_runs.push_back(std::move(line_runs));
                    } else {
                        skip_value();
                    }
                    skip_ws();
                    if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
                    else break;
                }
                expect(']');
            } else {
                skip_value();
            }
            skip_ws();
            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
            else break;
        }
        expect('}');
        if (page_runs.empty()) {
            page_runs.emplace_back(); // at least one (empty) paragraph
        }
        return page_runs;
    }

    // Top-level document parse
    // Returns per-page, per-paragraph, per-run data
    std::vector<std::vector<std::vector<editor::TextRun>>> parse_document() {
        std::vector<std::vector<std::vector<editor::TextRun>>> pages;
        expect('{');
        skip_ws();
        while (pos < src.size() && src[pos] != '}') {
            std::string key = parse_string();
            skip_ws();
            expect(':');
            skip_ws();
            if (key == "pages") {
                expect('[');
                skip_ws();
                while (pos < src.size() && src[pos] != ']') {
                    pages.push_back(parse_page());
                    skip_ws();
                    if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
                    else break;
                }
                expect(']');
            } else {
                skip_value();
            }
            skip_ws();
            if (pos < src.size() && src[pos] == ',') { ++pos; skip_ws(); }
            else break;
        }
        // consume closing '}'
        skip_ws();
        if (pos < src.size() && src[pos] == '}') ++pos;
        return pages;
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------------

bool SaveManager::load(editor::Document& document,
                       const std::filesystem::path& path,
                       std::string& error_message) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        error_message = "Unable to open file for reading.";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();

    // Determine format
    bool is_sord_json = (content.find("\"version\"") != std::string::npos &&
                         content.find("\"pages\"")   != std::string::npos);

    if (is_sord_json) {
        // Full JSON parse preserving per-run font information
        try {
            Parser parser{content, 0};
            auto all_pages = parser.parse_document();

            if (all_pages.empty()) {
                // Produce a single empty page
                all_pages.push_back({{}}); // one page, one paragraph, no runs
            }

            document.set_pages_from_runs(std::move(all_pages));
            return true;

        } catch (const std::exception& ex) {
            error_message = std::string("JSON parse error: ") + ex.what();
            return false;
        }
    } else {
        // Legacy plain-text format: one line per paragraph, no style info
        std::vector<std::string> lines;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            // strip trailing CR if any
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.emplace_back(std::move(line));
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
