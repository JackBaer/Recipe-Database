#ifndef MAINMENU_H
#define MAINMENU_H

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
#include "pdfExporter.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

void RenderSearchWindow(AppState& appState);
void RenderDisplayWindow(AppState& appState);

void ShowMainMenuPage(AppState& appState);	

#endif
