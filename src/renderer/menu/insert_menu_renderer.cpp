#include <renderer/menu/insert_menu_renderer.hpp>
#include <sstream>

namespace sord {
namespace renderer {
namespace menu {
    std:: string insertMenuRenderer::Render() {
        std::ostringstream oss;

        // First row: font selector, size, small controls (approximate layout)
        oss << "[Image] [Table] [Icons] [Symbol] [Equation] [Shapes] [Table]  [Take screenshot] [Page Number] [Comment]" << "\n";
        oss << "\n\n\n\n";

        //oss << "[Table]  [Take screenshot] [Page Number] [Comment]\n";
        
        oss << "\n";

        return oss.str();
    }
}
}
}
    //namespace insert menu