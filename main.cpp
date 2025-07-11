// Dear ImGui: standalone example application for SDL3 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#define IMGUI_DEFINE_MAKE_ENUMS  // Optional for enums, safe to include
#define IMGUI_HAS_DOCK           // Enable DockBuilder API (older ImGui versions)
#define IMGUI_ENABLE_DOCKING     // ✅ Required in recent versions
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>

#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <filesystem>                 
#include <fstream>
#include <algorithm> // for std::transform
#include <cctype>    // for std::tolower

#include "data.hpp"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

struct AppState {
	int counter = 0;
	std::vector<Ingredient> current_ingredients;
	std::string current_directions = "";
};

float parse_quantity_to_float(const std::string& str) {
    std::string s = str;
    s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());

    // Replace unicode fractions with float values
    static std::unordered_map<std::string, float> unicode_fractions = {
        {"¼", 0.25f}, {"½", 0.5f}, {"¾", 0.75f},
        {"⅓", 1.0f/3}, {"⅔", 2.0f/3}, {"⅛", 0.125f},
        {"⅜", 0.375f}, {"⅝", 0.625f}, {"⅞", 0.875f}
    };

    for (const auto& [sym, val] : unicode_fractions)
        if (s == sym) return val;

    try {
        size_t slash = s.find('/');
        if (slash != std::string::npos) {
            float num = std::stof(s.substr(0, slash));
            float denom = std::stof(s.substr(slash + 1));
            return num / denom;
        } else {
            return std::stof(s);
        }
    } catch (...) {
        return -1.0f;
    }
}

std::filesystem::path get_executable_directory(char* argv0) {
    try {
        return std::filesystem::canonical(argv0).parent_path();
    } catch (const std::exception& e) {
        std::cerr << "Could not resolve executable path: " << e.what() << "\n";
        return std::filesystem::current_path(); // fallback
    }
}

void ShowDockSpace()
{
    static bool initialized = false;

    // Create a full-screen, invisible window to host the dockspace
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("MainDockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    // Create the dockspace
    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Only build the layout once
    if (!initialized)
    {
        initialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        // Split the dockspace into 2 nodes: left and right (50%/50%)
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_left, dock_id_right;
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.5f, &dock_id_left, &dock_id_right);

        // Dock the windows
        ImGui::DockBuilderDockWindow("Search Window", dock_id_left);
        ImGui::DockBuilderDockWindow("Display Window", dock_id_right);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End(); // End of main dockspace window
}

// Main code
int main(int argc, char** argv)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(720 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // 💥 Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Our application state
    AppState appState;

    std::filesystem::path executable_dir = get_executable_directory(argv[0]);
    std::filesystem::path csv_path = executable_dir / "recipes.csv";
	

    read_recipes_from_csv(csv_path);


    //load_recipes(csv_path);
    
    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();


	ShowDockSpace();

	ImGui::Begin("Search Window");

	// SEARCH FIELD
	static char characterBuffer[40];
	ImGui::InputText("Dish Name", characterBuffer, IM_ARRAYSIZE(characterBuffer));

	// Convert input text to lowercase
	std::string currentText(characterBuffer);
	std::transform(currentText.begin(), currentText.end(), currentText.begin(),
		       [](unsigned char c){ return std::tolower(c); });
	static char buf1[32] = "";
	static char buf2[32] = "";
	static int selected_unit_idx = 0;  // <- Unit dropdown index

	/*
	// Narrow Items
	std::vector<std::pair<std::string, int>> currentRecipes;
	currentRecipes.clear();

	for (int i = 0; i < recipes.size(); ++i) {
	    std::string loweredName = recipes[i].name;
	    std::transform(loweredName.begin(), loweredName.end(), loweredName.begin(), [](unsigned char c){ return std::tolower(c); });

	    if(loweredName.find(currentText) != std::string::npos) {
		currentRecipes.emplace_back(recipes[i].name, i);
	    }
	}
	*/
	
	if(ImGui::TreeNode("Additional Filters")) {
		{
			if (ImGui::BeginChild("ConstrainedChild", ImVec2(-FLT_MIN, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY)) {
				ImGui::InputText("Ingredient", buf1, IM_ARRAYSIZE(buf1));	
				ImGui::SameLine();
				ImGui::InputText("Quantity", buf2, IM_ARRAYSIZE(buf2));
	
				if (!availableUnits.empty()) {
				    const char* preview = availableUnits[selected_unit_idx].c_str();
				    if (ImGui::BeginCombo("Unit", preview)) {
					for (int i = 0; i < availableUnits.size(); ++i) {
					    bool is_selected = (selected_unit_idx == i);
					    if (ImGui::Selectable(availableUnits[i].c_str(), is_selected))
						selected_unit_idx = i;
					    if (is_selected)
						ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				    }
				}
				ImGui::EndChild();
			}
		}

		ImGui::TreePop();
	}

	std::string filterIngredient(buf1);
	std::string filterQuantity(buf2);

	// Lowercase and trim helper
	auto normalize = [](std::string& s) {
	    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
	    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
	};

	normalize(filterIngredient);
	normalize(filterQuantity);

	// Build the filtered list
	std::vector<std::pair<std::string, int>> currentRecipes;
	currentRecipes.clear();

	for (int i = 0; i < recipes.size(); ++i) {
	    std::string loweredName = recipes[i].name;
	    std::transform(loweredName.begin(), loweredName.end(), loweredName.begin(), [](unsigned char c){ return std::tolower(c); });

	    // Match dish name
	    if (loweredName.find(currentText) == std::string::npos)
		continue;

	    // Match ingredients/quantities (if filters are active)
	    bool passesIngredientFilter = true;

	    if (!filterIngredient.empty() || !filterQuantity.empty()) {
		passesIngredientFilter = false;
		for (const auto& ing : recipes[i].ingredients) {
		    std::string name = ing.name;
		    std::string qty = ing.quantity;
		    normalize(name);
		    normalize(qty);

		    bool matchIngredient = filterIngredient.empty() || name.find(filterIngredient) != std::string::npos;
		    bool matchQuantity = filterQuantity.empty() || qty.find(filterQuantity) != std::string::npos;

		    if (matchIngredient && matchQuantity) {
			passesIngredientFilter = true;
			break;
		    }
		}
	    }

	    if (passesIngredientFilter) {
		currentRecipes.emplace_back(recipes[i].name, i);
	    }
	    }

	/*
	std::string filterIngredient(buf1);
std::string filterQuantity(buf2);
std::string filterUnit = availableUnits.empty() ? "" : availableUnits[selected_unit_idx];

auto normalize = [](std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end());
};

normalize(filterIngredient);
normalize(filterQuantity);
normalize(filterUnit);

	// Build the filtered list
	std::vector<std::pair<std::string, int>> currentRecipes;
	currentRecipes.clear();



float quantityFilter = parse_quantity_to_float(filterQuantity);
std::vector<std::tuple<std::string, int, float>> matchedRecipes;

for (int i = 0; i < recipes.size(); ++i) {
    std::string loweredName = recipes[i].name;
    std::transform(loweredName.begin(), loweredName.end(), loweredName.begin(), ::tolower);

    if (loweredName.find(currentText) == std::string::npos)
        continue;

    float bestQtyMatch = -1.0f;
    bool matched = false;

    for (const auto& ing : recipes[i].ingredients) {
        std::string ingName = ing.name;
        std::string ingUnit = ing.unit;
        std::string ingQty = ing.quantity;

        normalize(ingName);
        normalize(ingUnit);
        normalize(ingQty);

        float ingQtyFloat = parse_quantity_to_float(ingQty);

        bool matchesName = filterIngredient.empty() || ingName.find(filterIngredient) != std::string::npos;
        bool matchesUnit = filterUnit.empty() || ingUnit == filterUnit;
        bool matchesQty = filterQuantity.empty() || (ingQtyFloat >= 0 && ingQtyFloat <= quantityFilter);

        if (matchesName && matchesUnit && matchesQty) {
            matched = true;
            if (ingQtyFloat > bestQtyMatch)
                bestQtyMatch = ingQtyFloat;
        }
    }

    if (matched)
        matchedRecipes.emplace_back(recipes[i].name, i, bestQtyMatch);
}

std::sort(matchedRecipes.begin(), matchedRecipes.end(),
          [](const auto& a, const auto& b) {
              return std::get<2>(a) > std::get<2>(b);
          });

currentRecipes.clear();
for (const auto& [name, index, _] : matchedRecipes)
    currentRecipes.emplace_back(name, index);
*/

	// FILTERED LISTBOX
	static int item_selected_idx = 0; // Here we store our selected data as an index.
        static bool item_highlight = false;
        int item_highlighted_idx = -1; // Here we store our highlighted data as an index.

	// Get the remaining vertical space in the current window
        float available_height = ImGui::GetContentRegionAvail().y;

        // Custom size: use all width, 5 items tall
	ImGui::Text("Recipes:");

	/*
	if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, available_height)))
	{
	    
	for(int n = 0; n < currentRecipes.size(); ++n) {
		const auto& [name, originalIndex] = currentRecipes[n];
		bool is_selected = (item_selected_idx == n);

		ImGuiSelectableFlags flags = (item_highlighted_idx == n) ? ImGuiSelectableFlags_Highlight : 0;

		//std::string label = recipe_names[n] + "###recipe_" + std::to_string(n);
		std::string label = name + "###recipe_" + std::to_string(originalIndex);
		if (ImGui::Selectable(label.c_str(), is_selected, flags))
		    item_selected_idx = n;

		if (is_selected) {
		    ImGui::SetItemDefaultFocus();

		    //appState.current_ingredients = ingredients[originalIndex];
		    //appState.current_directions = directions[originalIndex];
		
		    appState.current_ingredients = recipes[originalIndex].ingredients;
    		    appState.current_directions = recipes[originalIndex].directions;	
		}
		//std::cout << "Current Number: " << item_selected_idx;		
	    }
	    ImGui::EndListBox();

        }
	*/

	if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, available_height))) {
	    for (int n = 0; n < currentRecipes.size(); ++n) {
		const auto& [name, originalIndex] = currentRecipes[n];
		bool is_selected = (item_selected_idx == n);
		ImGuiSelectableFlags flags = (item_highlighted_idx == n) ? ImGuiSelectableFlags_Highlight : 0;

		std::string label = name + "###recipe_" + std::to_string(originalIndex);
		if (ImGui::Selectable(label.c_str(), is_selected, flags))
		    item_selected_idx = n;

		if (is_selected) {
		    ImGui::SetItemDefaultFocus();
		    appState.current_ingredients = recipes[originalIndex].ingredients;
		    appState.current_directions = recipes[originalIndex].directions;
		}
	    }
	    ImGui::EndListBox();
	}
			
	ImGui::End();

	ImGui::Begin("Display Window"); 


	std::string full_ingredients = clean_and_format_ingredients(appState.current_ingredients);
	//std::cout << full_ingredients;

	ImGui::TextWrapped(full_ingredients.c_str());
	
	ImGui::NewLine();
	ImGui::TextWrapped(appState.current_directions.c_str());

	ImGui::End();


        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
