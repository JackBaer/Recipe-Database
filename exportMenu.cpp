#include "exportMenu.h"

void ShowExportPage(AppState& appState) {
    ImGui::Begin("Export Page");

    // Static texture variables
    static GLuint previewTextureID = 0;
    static int previewTexWidth = 0;
    static int previewTexHeight = 0;
    static bool textureLoaded = false;
    
if (!textureLoaded) {
        previewTextureID = LoadTextureGL("preview.png", &previewTexWidth, &previewTexHeight);
        textureLoaded = true;
    }

    if (previewTextureID) {
        float scale = 0.5f;
        ImGui::Text("Preview of Exported PDF:");
        ImGui::Image((ImTextureID)(intptr_t)previewTextureID,
                     ImVec2(previewTexWidth * scale, previewTexHeight * scale));
    } else {
        ImGui::Text("Failed to load preview image.");
    }

    if (ImGui::Button("Download PDF")) {
        SaveRecipePDF(appState);
        ImGui::OpenPopup("Success");
    }

    if (ImGui::BeginPopupModal("Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("PDF has been saved successfully!");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (ImGui::Button("Back to Main Menu")) {
        appState.currentPage = Page::MainMenu;
    }

    ImGui::End();
}
