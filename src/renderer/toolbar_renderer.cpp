#include "renderer/toolbar_renderer.hpp"

#include <sstream>

namespace sord {
namespace renderer {

std::vector<std::string> ToolbarRenderer::Titles() {
    return {
        "Home",
        "Insert",
        "Draw",
        //"Paging",
        "Graph/Charts",
        //"Header",
        //"Footer",
    };
}

std::string ToolbarRenderer::TabLabel(Tab tab) {
    switch (tab) {
        case Tab::Home:
            return "Home ";
        case Tab::Insert:
            return "Insert ";
        case Tab::Draw:
            return "Draw ";
        //case Tab::Paging:
            //return "Paging ";
        case Tab::GraphCharts:
            return "Graph/Charts ";
        //case Tab::Header:
            //return "Header ";
        //case Tab::Footer:
            //return "Footer ";
    }
    return "Unknown";
}

std::string ToolbarRenderer::RenderTitleBar(Tab selected_tab) {
    auto labels = Titles();
    std::ostringstream oss;
    for (std::size_t index = 0; index < labels.size(); ++index) {
        if (index != 0) {
            oss << "  ";
        }
        if (static_cast<Tab>(index) == selected_tab) {
            oss << "[" << labels[index] << "] ";
        } else {
            oss << labels[index];
        }
    }
    return oss.str();
}

std::string ToolbarRenderer::RenderSubToolbar(Tab selected_tab) {
    std::ostringstream oss;
    //oss << "Sub Toolbar Placeholder\n";
    //oss << "Selected: " << TabLabel(selected_tab) << "\n";
    oss <<"--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n";
    //oss << "Feature options will appear here.\n";
    oss << "\n";
    oss << "\n";
    oss << "\n";
    oss << "\n";
    //oss << "\n";
    return oss.str();
}

std::string ToolbarRenderer::RenderToolbar(Tab selected_tab) {
    std::ostringstream oss;
    oss << RenderTitleBar(selected_tab) << "\n";
    oss << std::string(160, '-') << "\n";
    oss << RenderSubToolbar(selected_tab);
    return oss.str();
}

}  // namespace renderer
}  // namespace sord
