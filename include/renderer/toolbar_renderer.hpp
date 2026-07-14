#pragma once

#include <string>
#include <vector>

namespace sord {
namespace renderer {

class ToolbarRenderer {
public:
    enum class Tab {
        Home = 0,
        Insert,
        Draw,
        Paging,
        GraphCharts,
        Header,
        Footer,
    };

    static std::vector<std::string> Titles();
    static std::string RenderToolbar(Tab selected_tab);
    static std::string RenderTitleBar(Tab selected_tab);
    static std::string RenderSubToolbar(Tab selected_tab);
    static std::string TabLabel(Tab tab);
};

}  // namespace renderer
}  // namespace sord
