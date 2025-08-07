#define IMGUI_DEFINE_MAKE_ENUMS  // Optional for enums, safe to include
#define IMGUI_HAS_DOCK           // Enable DockBuilder API (older ImGui versions)

#ifndef IMGUI_ENABLE_DOCKING
#define IMGUI_ENABLE_DOCKING     // ✅ Required in recent versions
#endif

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
#include "appState.h" // container struct for containing all persistent data
#include "mainMenu.h" // GUI methods for main menu
#include "exportMenu.h" // GUI methods for export menu
#include "recipeCreateMenu.h" // GUI methods for recipe creator window
#include "pdfExporter.h" // Logic for exporting recipe into a PDF file

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

AppState appState;

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

	    // Now split right into top and bottom
	    ImGuiID topRight, bottomRight;
	    ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.07f, &bottomRight, &topRight);

            ImGui::DockBuilderDockWindow("Search Window", left);
            ImGui::DockBuilderDockWindow("Display Window", topRight);
	    ImGui::DockBuilderDockWindow("Main Menu Controls", bottomRight);
	}
        else if (currentPage == Page::RecipeCreate) {
            // Reserved for future layout
	    ImGuiID top, bottom;
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.87f, &top, &bottom);

            ImGui::DockBuilderDockWindow("Recipe Creator Window", top);
	    ImGui::DockBuilderDockWindow("Main Menu Controls", bottom);
        }
        else if (currentPage == Page::ExportRecipe) {
            // Reserved for future layout
            ImGuiID left, right;
	    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, &left, &right);
	    
	    // Now split right into top and bottom
	    ImGuiID topRight, bottomRight;
	    ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.07f, &bottomRight, &topRight);

	    ImGui::DockBuilderDockWindow("Export Window", left);
	    ImGui::DockBuilderDockWindow("Display Window", topRight);
	    ImGui::DockBuilderDockWindow("Main Menu Controls", bottomRight);
	}

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
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
	ShowDockSpace(appState.currentPage);

	switch (appState.currentPage) {
	    case Page::MainMenu:
		ShowMainMenuPage(appState);
		break;
	    case Page::RecipeCreate:
		ShowRecipeCreatePage(appState);  // You can leave this empty for now
		break;
	    case Page::ExportRecipe:
		ShowExportPage(appState);        // Empty for now
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
