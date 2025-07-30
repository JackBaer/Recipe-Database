#include "mainMenu.h"

void RenderSearchWindow(AppState& appState) {
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

void RenderDisplayWindow(AppState& appState) {
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

void ShowMainMenuPage(AppState& appState) {
    RenderSearchWindow(appState);
    RenderDisplayWindow(appState);

    ImGui::Begin("Main Menu Controls");

    
    if (ImGui::Button("Export Recipe")) {
        appState.currentPage = Page::ExportRecipe;
    }
    
    ImGui::End();
}
