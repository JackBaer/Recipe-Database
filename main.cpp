#include "hello_imgui/hello_imgui.h"
#include "immapp/immapp.h"

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <filesystem>                 // C++17
#include <fstream>
	
#include "data.hpp"

struct AppState {
	int counter = 0;
	std::string current_ingredients = "";
	std::string current_directions = "";
};

std::filesystem::path get_executable_directory(char* argv0) {
    try {
        return std::filesystem::canonical(argv0).parent_path();
    } catch (const std::exception& e) {
        std::cerr << "Could not resolve executable path: " << e.what() << "\n";
        return std::filesystem::current_path(); // fallback
    }
}

void ShowSearchWindow(AppState& appState) {
if (ImGui::TreeNode("Recipes"))
    {
	int item_selected_idx = -1; 

	ImVec2 size = ImGui::GetWindowSize();
	float height = size.y;
	int num_lines = height * ImGui::GetTextLineHeightWithSpacing();	
        
	if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, num_lines)))
        {
		for (int n = 0; n < recipe_names.size(); n++)
            {
                bool is_selected = (item_selected_idx == n);
		//ImGuiSelectableFlags flags = (item_highlighted_idx == n) ? ImGuiSelectableFlags_Highlight : 0;
                if (ImGui::Selectable(recipe_names[n].c_str(), is_selected)) {
                    item_selected_idx = n;
		}

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
   			appState.current_ingredients = ingredients[n];
			appState.current_directions = directions[n];
		}
	}
        ImGui::EndListBox();
        }

        ImGui::TreePop();
    }
}

void ShowDisplayWindow(AppState& appState) {
	ImGui::Text(appState.current_ingredients.c_str());
	ImGui::Text(appState.current_directions.c_str());
}

std::vector<HelloImGui::DockingSplit> CreateDefaultDockingSplits() {
	HelloImGui::DockingSplit split;

	split.initialDock = "MainDockSpace";
	split.direction = ImGuiDir_Left;
	split.ratio = 0.5f;
	split.newDock = "Recipe Space";
	
	std::vector<HelloImGui::DockingSplit> splitList;
	splitList.push_back(split);
	return splitList;
}

std::vector<HelloImGui::DockableWindow> CreateDockableWindows(AppState& appState) {
	HelloImGui::DockableWindow searchWindow;
	searchWindow.label = "SearchWindow";
	searchWindow.dockSpaceName = "MainDockSpace";
	searchWindow.GuiFunction = [&] { ShowSearchWindow(appState); };


	HelloImGui::DockableWindow displayWindow;
	displayWindow.label = "DisplayWindow";
	displayWindow.dockSpaceName = "MainDockSpace";
	displayWindow.GuiFunction = [&] { ShowDisplayWindow(appState); };

	std::vector<HelloImGui::DockableWindow> dockableWindows {
		searchWindow,
		displayWindow
	};

	return dockableWindows;
}


HelloImGui::DockingParams CreateDefaultLayout(AppState& appState) {
	HelloImGui::DockingParams dockingParams;
	dockingParams.dockingSplits = CreateDefaultDockingSplits();
	dockingParams.dockableWindows = CreateDockableWindows(appState);
	return dockingParams;
}

int main(int argc, char* argv[]) {
	std::filesystem::path executable_dir = get_executable_directory(argv[0]);
std::filesystem::path csv_path = executable_dir / "recipes.csv";
	
	load_recipes(csv_path);

	// Our application state
	AppState appState;

	// Hello ImGui params (they hold the settings as well as the Gui callbacks)
	HelloImGui::RunnerParams runnerParams;

	runnerParams.appWindowParams.windowTitle = "Recipe Search";
	runnerParams.imGuiWindowParams.menuAppTitle = "App Test";
	runnerParams.appWindowParams.windowGeometry.size = {1000, 900};
	runnerParams.appWindowParams.restorePreviousGeometry = true;
	//runnerParams.appWindowParams.borderless = true;
	//runnerParams.appWindowParams.borderlessMovable = true;
	//runnerParams.appWindowParams.borderlessResizable = true;
	//runnerParams.appWindowParams.borderlessClosable = true;

	// First, tell HelloImGui that we want full screen dock space (this will create "MainDockSpace")
	runnerParams.imGuiWindowParams.defaultImGuiWindowType = HelloImGui::DefaultImGuiWindowType::ProvideFullScreenDockSpace;
	// Set the default layout (this contains the default DockingSplits and DockableWindows)
	runnerParams.dockingParams = CreateDefaultLayout(appState);

	HelloImGui::Run(runnerParams); // Note: with ImGuiBundle, it is also possible to use ImmApp::Run(...)
	return 0;
}


