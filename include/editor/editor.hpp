#pragma once

#include <memory>
#include <string>

#include "editor/document.hpp"

namespace sord {
namespace editor {

class Editor {
public:
    explicit Editor(std::string title = "Untitled.sord");

    void handle_input(int key);
    void handle_char(const std::string& text);

    [[nodiscard]] Document& document();
    [[nodiscard]] const Document& document() const;

private:
    Document document_;
};

using EditorPtr = std::shared_ptr<Editor>;

}  // namespace editor
}  // namespace sord
