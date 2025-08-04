#include "recipeCreateMenu.h"

std::string EscapeCSVField(const std::string& input) {
    std::string escaped = "\"";
    for (char c : input) {
        if (c == '"') {
            escaped += "\"\"";  // escape double quotes
        } else {
            escaped += c;
        }
    }
    escaped += "\"";
    return escaped;
}

void AppendRecipeToCSV(const Recipe& recipe, const std::string& filename) {
    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open recipe CSV for appending.\n";
        return;
    }

    // Build ingredient string
    std::ostringstream ingStream;
    for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
        const auto& ing = recipe.ingredients[i];
        ingStream << ing.quantity << "~" << ing.unit << "~" << ing.name;
        if (i < recipe.ingredients.size() - 1) ingStream << "|";
    }

    // Initialize 15 columns (0-indexed, so column 1 = index 0)
    std::vector<std::string> columns(15, "");

    columns[1] = EscapeCSVField(recipe.name);            // Column 2
    columns[4] = EscapeCSVField(recipe.time + "mins");   // Column 5
    columns[7] = EscapeCSVField(ingStream.str());        // Column 8
    columns[8] = EscapeCSVField(recipe.directions);      // Column 9

    // Output the full line with 14 commas (15 columns)
    for (size_t i = 0; i < columns.size(); ++i) {
        file << columns[i];
        if (i < columns.size() - 1)
            file << ",";
    }
    file << "\n";

    file.close();
}

void ShowRecipeCreatePage(AppState& appState) {

    ImGui::Begin("Recipe Creator Window");

    static bool showAddRecipeWindow = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Add New Recipe")) {
                showAddRecipeWindow = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (showAddRecipeWindow) {
        static char recipeName[256] = "";
        static char totalTime[64] = "";
        static char directions[2048] = "";

	// Inputs for one ingredient
	static char ingredientQuantity[64] = "";

 	static int selected_unit_idx = 0;

	static char ingredientName[256] = "";
        
	// Vector to hold ingredients as user adds them
	static std::vector<Ingredient> newIngredients;

	if (showAddRecipeWindow) {
    ImGui::Begin("Add New Recipe", &showAddRecipeWindow);

    ImGui::InputText("Recipe Name", recipeName, IM_ARRAYSIZE(recipeName));
    ImGui::InputText("Total Time (minutes)", totalTime, IM_ARRAYSIZE(totalTime), ImGuiInputTextFlags_CharsDecimal);

    ImGui::Separator();
    ImGui::Text("Add Ingredient:");

    ImGui::InputText("Quantity", ingredientQuantity, IM_ARRAYSIZE(ingredientQuantity), ImGuiInputTextFlags_CharsDecimal);
//    ImGui::InputText("Unit", ingredientUnit, IM_ARRAYSIZE(ingredientUnit));
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

    ImGui::InputText("Name", ingredientName, IM_ARRAYSIZE(ingredientName));

    if (ImGui::Button("Add Ingredient")) {
        if (strlen(ingredientName) > 0) { // simple validation
            newIngredients.push_back(Ingredient{
                ingredientQuantity,
                ingredientName,
	    	availableUnits[selected_unit_idx]
            });
            // Clear inputs for next ingredient
            ingredientQuantity[0] = '\0';
            //ingredientUnit[0] = '\0';
            ingredientName[0] = '\0';
        }
    }

    // Show list of ingredients added so far
    ImGui::Separator();
    ImGui::Text("Ingredients:");
    for (size_t i = 0; i < newIngredients.size(); i++) {
        ImGui::Text("%s %s %s",
                    newIngredients[i].quantity.c_str(),
                    newIngredients[i].unit.c_str(),
                    newIngredients[i].name.c_str());
    }

    ImGui::Separator();

    // Directions multiline text as before
    static char directions[2048] = "";
    ImGui::InputTextMultiline("Directions", directions, IM_ARRAYSIZE(directions), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 8));

    if (ImGui::Button("Add Recipe")) {
	    // Construct recipe from inputs
	    Recipe newRecipe;
	    newRecipe.name = recipeName;
	    newRecipe.time = totalTime;
	    newRecipe.ingredients = newIngredients;
	    newRecipe.directions = directions;

	    // Update app state
	    //appState.recipes.push_back(newRecipe);

	    // Save to CSV
	    AppendRecipeToCSV(newRecipe, "recipes.csv");  // adjust path if needed

	    // Clear form fields
	    recipeName[0] = '\0';
	    totalTime[0] = '\0';
	    directions[0] = '\0';
	    newIngredients.clear();

	    showAddRecipeWindow = false;
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
	showAddRecipeWindow = false;
    }

	    ImGui::End();
	}
    }
   
    ImGui::End();

    ImGui::Begin("Main Menu Controls");

    if (ImGui::Button("Export Recipe as PDF")) {
	appState.currentPage = Page::ExportRecipe;  // Navigate to preview screen
    } 

    ImGui::SameLine();

    if (ImGui::Button("Back to Main Menu")) {
        appState.currentPage = Page::MainMenu;
    }

    ImGui::End();
}
