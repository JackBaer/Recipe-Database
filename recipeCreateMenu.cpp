#include "recipeCreateMenu.h"

void ShowRecipeCreatePage(AppState& appState) {

    ImGui::Begin("Recipe Creator");

    if (ImGui::Button("Back to Main Menu")) {
        appState.currentPage = Page::MainMenu;
    }

    ImGui::End();
}
