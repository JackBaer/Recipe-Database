#define IMGUI_DEFINE_MAKE_ENUMS  // Optional for enums, safe to include
#define IMGUI_HAS_DOCK           // Enable DockBuilder API (older ImGui versions)
#define IMGUI_ENABLE_DOCKING     // ✅ Required in recent versions

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <sstream>
#include <filesystem>                 
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <map>
#include <algorithm> // for std::transform
#include <cctype>    // for std::tolower
#include <regex>

#include "data.hpp" // outsourced helper methods for parsing CSV data

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

enum Page {
	MainMenu,
	RecipeCreate,
	ExportRecipe
};

struct AppState {
	// Values for Display menu, preserved across frames
	std::string current_recipe;
	std::vector<Ingredient> current_ingredients;
	std::string current_directions = "";
	ImFont* font_normal = nullptr;
	ImFont* font_large = nullptr;
};

Page currentPage = Page::MainMenu;

AppState appState;

//
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

// Ensure generated executable is able to locate CSV file
std::filesystem::path get_executable_directory(char* argv0) {
    try {
        return std::filesystem::canonical(argv0).parent_path();
    } catch (const std::exception& e) {
        std::cerr << "Could not resolve executable path: " << e.what() << "\n";
        return std::filesystem::current_path(); // fallback
    }
}

void BuildDockLayoutForPage(ImGuiID dockspace_id, Page page, const ImVec2& size)
{
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, size);

    if (page == Page::RecipeCreate)
    {
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_left, dock_id_right;
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.5f, &dock_id_left, &dock_id_right);

        ImGui::DockBuilderDockWindow("Search Window", dock_id_left);
        ImGui::DockBuilderDockWindow("Display Window", dock_id_right);
        ImGui::DockBuilderDockWindow("Recipe App", dock_main_id);
    }
    else if (page == Page::ExportRecipe)
    {
        ImGui::DockBuilderDockWindow("Export Recipe", dockspace_id);
    }
    else if (page == Page::MainMenu)
    {
        ImGui::DockBuilderDockWindow("Main Menu Window", dockspace_id);
    }

    ImGui::DockBuilderFinish(dockspace_id);
}

void ShowDockSpace(Page currentPage) {
    static bool initialized = false;
    static Page lastPage = Page::MainMenu;

    // Get main viewport
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

    // Rebuild layout only if the page changes
    if (!initialized || currentPage != lastPage) {
        initialized = true;
        lastPage = currentPage;

        // Clear any previous layout
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        if (currentPage == Page::MainMenu) {
            ImGuiID left, right;
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, &left, &right);

            ImGui::DockBuilderDockWindow("Search Window", left);
            ImGui::DockBuilderDockWindow("Display Window", right);
        }
        else if (currentPage == Page::RecipeCreate) {
            // Reserved for future layout
        }
        else if (currentPage == Page::ExportRecipe) {
            // Reserved for future layout
        }

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();
}

void RenderSearchWindow() {
    ImGui::PushFont(appState.font_normal);
    ImGui::Begin("Search Window");

    // Dish name input
    static char dishName[40] = "";
    ImGui::InputText("Dish Name", dishName, IM_ARRAYSIZE(dishName));

    std::string currentText(dishName);
    std::transform(currentText.begin(), currentText.end(), currentText.begin(),
        [](unsigned char c){ return std::tolower(c); });

    static char ingredientName[32] = "";
    static char ingredientQuantity[32] = "";
    static char recipeTime[32] = "";
    static int selected_unit_idx = 0;
    static bool include_less_equal = false; // new toggle

    if (ImGui::TreeNode("Additional Filters")) {
        if (ImGui::BeginChild("FilterChild", ImVec2(-FLT_MIN, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY)) {
            ImGui::InputText("Ingredient Name", ingredientName, IM_ARRAYSIZE(ingredientName));
            ImGui::InputText("Ingredient Quantity", ingredientQuantity, IM_ARRAYSIZE(ingredientQuantity));

            if (!availableUnits.empty()) {
                const char* preview = availableUnits[selected_unit_idx].c_str();
                if (ImGui::BeginCombo("Ingredient Unit", preview)) {
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

            ImGui::InputText("Recipe Time", recipeTime, IM_ARRAYSIZE(recipeTime));
            ImGui::Checkbox("Include recipes with lesser quantity", &include_less_equal);
	    ImGui::EndChild();
        }
        ImGui::TreePop();
    }

    // Filter logic & result listbox...
    // (keep the filtering/normalizing and listbox code from your original block here)
	std::string filterIngredient(ingredientName);
	std::string filterQuantity(ingredientQuantity);
	std::string filterUnit(availableUnits[selected_unit_idx]);
	
	// Lowercase and trim helper
	auto normalize = [](std::string& s) {
	    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
	    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
	    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
	};

	normalize(filterIngredient);
	normalize(filterQuantity);
	normalize(filterUnit);

	// Convert ASCII fraction in filterQuantity to Unicode
	std::unordered_map<std::string, std::string> ascii_to_unicode = {
	    {"1/4", "¼"}, {"1/2", "½"}, {"3/4", "¾"},
	    {"1/3", "⅓"}, {"2/3", "⅔"},
	    {"1/5", "⅕"}, {"2/5", "⅖"}, {"3/5", "⅗"}, {"4/5", "⅘"},
	    {"1/6", "⅙"}, {"5/6", "⅚"},
	    {"1/8", "⅛"}, {"3/8", "⅜"}, {"5/8", "⅝"}, {"7/8", "⅞"}
	};

	// Handle "1 1/2" → "1½"
	std::regex mixed_number_pattern(R"((\b\d+)\s+(\d/\d)\b)");
	std::smatch match;
	while (std::regex_search(filterQuantity, match, mixed_number_pattern)) {
	    std::string whole = match[1];
	    std::string frac = match[2];
	    auto it = ascii_to_unicode.find(frac);
	    if (it != ascii_to_unicode.end()) {
		filterQuantity = filterQuantity.substr(0, match.position()) +
				 whole + it->second +
				 filterQuantity.substr(match.position() + match.length());
	    } else {
		break;
	    }
	}

	// Handle standalone "1/2" → "½"
	for (const auto& [ascii_frac, unicode_frac] : ascii_to_unicode) {
	    std::regex standalone_frac(R"(\b)" + ascii_frac + R"(\b)");
	    filterQuantity = std::regex_replace(filterQuantity, standalone_frac, unicode_frac);
	}

	// Build the filtered list
	std::vector<std::pair<std::string, int>> currentRecipes;
	std::vector<std::pair<std::string, std::pair<int, double>>> sortedMatches;

	double targetQty = filterQuantity.empty() ? -1.0 : parse_mixed_fraction(filterQuantity);


	for (int i = 0; i < recipes.size(); ++i) {
	    std::string loweredName = recipes[i].name;
	    std::transform(loweredName.begin(), loweredName.end(), loweredName.begin(), [](unsigned char c){ return std::tolower(c); });

	    if (loweredName.find(currentText) == std::string::npos) continue;

	    if (filterIngredient.empty() && filterQuantity.empty() && (filterUnit == " ")) {
		currentRecipes.emplace_back(recipes[i].name, i);
	    } else {
		bool matchFound = false;
		double bestQty = -1.0;

		for (const auto& ing : recipes[i].ingredients) {
		    std::string name = ing.name;
		    std::string qty = ing.quantity;
		    std::string unit = ing.unit;
		    normalize(name); normalize(qty); normalize(unit);

		    double ingQty = qty.empty() ? -1.0 : parse_mixed_fraction(qty);

		    bool matchIngredient = filterIngredient.empty() || name.find(filterIngredient) != std::string::npos;
		    bool matchUnit = (filterUnit == " ") || unit.find(filterUnit) != std::string::npos;

		    if (include_less_equal) {
			if (matchIngredient && matchUnit && targetQty >= 0 && ingQty <= targetQty) {
			    matchFound = true;
			    bestQty = std::max(bestQty, ingQty); // track best match for sort
			}
		    } else {
			bool matchQuantity = filterQuantity.empty() || qty.find(filterQuantity) != std::string::npos;
			if (matchIngredient && matchQuantity && matchUnit) {
			    matchFound = true;
			    bestQty = ingQty;
			}
		    }
		}

		if (matchFound) {
		    if (include_less_equal)
			sortedMatches.emplace_back(recipes[i].name, std::make_pair(i, bestQty));
		    else
			currentRecipes.emplace_back(recipes[i].name, i);
		}
	    }
	}

	// If in inequality mode, sort by matched quantity descending
	if (include_less_equal) {
	    std::sort(sortedMatches.begin(), sortedMatches.end(),
		      [](const auto& a, const auto& b) {
			  return a.second.second > b.second.second;
		      });
	    for (const auto& entry : sortedMatches) {
		currentRecipes.emplace_back(entry.first, entry.second.first);
	    }
	}

	// FILTERED LISTBOX
	static int item_selected_idx = 0; // Selected entry as an index.
        int item_highlighted_idx = -1; // Highlighted entry as an index.

	// Get the remaining vertical space in the current window
        float available_height = ImGui::GetContentRegionAvail().y;

	ImGui::Text("Recipes:");

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
		    appState.current_recipe = recipes[originalIndex].name;
		    appState.current_ingredients = recipes[originalIndex].ingredients;
		    appState.current_directions = recipes[originalIndex].directions;
		}
	    }
	    ImGui::EndListBox();
	}
			
    ImGui::End();
    ImGui::PopFont();
}

void RenderDisplayWindow() {
	ImGui::Begin("Display Window");

    ImGui::PushFont(appState.font_large);
    ImGui::TextWrapped(appState.current_recipe.c_str());
    ImGui::PopFont();
    ImGui::PushFont(appState.font_normal);

    std::string full_ingredients = clean_and_format_ingredients(appState.current_ingredients);
    std::string full_directions = clean_recipe_directions(appState.current_directions);

    static std::string last_directions;
    static std::vector<std::string> direction_steps;
    static std::vector<char> step_checkboxes;

    if (full_directions != last_directions) {
        last_directions = full_directions;
        direction_steps = split_numbered_steps(full_directions);
        step_checkboxes = std::vector<char>(direction_steps.size(), 0);
    }

    ImGui::NewLine();
    ImGui::TextWrapped(full_ingredients.c_str());
    ImGui::NewLine();

    for (size_t i = 0; i < direction_steps.size(); ++i) {
        std::string label = "##step" + std::to_string(i);
        ImGui::Checkbox(label.c_str(), reinterpret_cast<bool*>(&step_checkboxes[i]));
        ImGui::SameLine();
        ImGui::Text("%s", direction_steps[i].c_str());
    }

    ImGui::PopFont();
    ImGui::End();
}

void ShowRecipeCreatePage() {

}

void ShowExportPage() {
    ImGui::Begin("Export Page");

    if (ImGui::Button("Back to Main Menu")) {
        currentPage = Page::MainMenu;
    }

    ImGui::End();
}

void ShowMainMenuPage() {
    RenderSearchWindow();
    RenderDisplayWindow();

    ImGui::Begin("Main Menu Controls");

    if (ImGui::Button("Export Recipe")) {
        currentPage = Page::ExportRecipe;
    }

    ImGui::End();
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
    SDL_Window* window = SDL_CreateWindow("Dynamic Pantry Recipe Engine", (int)(1280 * main_scale), (int)(720 * main_scale), window_flags);
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

    appState.font_normal = io.Fonts->AddFontFromFileTTF("Aver.ttf", 16.0f);
    appState.font_large = io.Fonts->AddFontFromFileTTF("Aver.ttf", 32.0f);

        // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    std::filesystem::path executable_dir = get_executable_directory(argv[0]);
    std::filesystem::path csv_path = executable_dir / "recipes.csv";
	
    read_recipes_from_csv(csv_path);

    clean_all_ingredients_in_recipes();
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
	// Listen for events
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

	// Build dockspace
	ShowDockSpace(currentPage);

	switch (currentPage) {
	    case Page::MainMenu:
		ShowMainMenuPage();
		break;
	    case Page::RecipeCreate:
		ShowRecipeCreatePage();  // You can leave this empty for now
		break;
	    case Page::ExportRecipe:
		ShowExportPage();        // Empty for now
		break;
	}

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
