#include "exportMenu.h"

void ShowExportPage(AppState& appState) {
    ImGui::Begin("Export Page");

    if (ImGui::Button("Back to Main Menu")) {
        appState.currentPage = Page::MainMenu;
    }

    ImGui::End();
}
