#include "recipeCreateMenu.h"

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
        // Convert newIngredients vector to a single string or store in your Recipe directly
        // Append to CSV and add to your recipes vector
        // Clear everything
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
