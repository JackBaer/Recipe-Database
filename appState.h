#ifndef APPSTATE_H
#define APPSTATE_H

#include "data.hpp"

#include "imgui.h"

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
	Page previousPage;
	Page currentPage = Page::MainMenu;
	ImFont* font_normal = nullptr;
	ImFont* font_large = nullptr;
};

#endif
