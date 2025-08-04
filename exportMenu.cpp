#include "exportMenu.h"
#include "mainMenu.h" // For reused Display Window
		      
void ShowExportPage(AppState& appState) {
    ImGui::Begin("Export Window");

    static std::string lastRecipeRendered;
    static GLuint previewTextureID = 0;
    static int previewTexWidth = 0;
    static int previewTexHeight = 0;

    // Persistent user input for filename
    static char filenameBuffer[128] = "recipe_export";
    
    // Input text field for filename
    ImGui::Text("Enter filename:");
    ImGui::InputText("##FilenameInput", filenameBuffer, IM_ARRAYSIZE(filenameBuffer));

    std::string filenameStr = filenameBuffer;

// If the recipe has changed, regenerate PDF & preview
    if (appState.current_recipe != lastRecipeRendered) {
        lastRecipeRendered = appState.current_recipe;

        SaveRecipePDF(appState, filenameStr);
        ConvertPDFToPNG(filenameStr + ".pdf", filenameStr);

        if (previewTextureID) {
            glDeleteTextures(1, &previewTextureID);
            previewTextureID = 0;
        }

        previewTextureID = LoadTextureGL((filenameStr + ".png").c_str(), &previewTexWidth, &previewTexHeight);
    }

    ImGui::Text("Preview of Exported PDF:");
    if (previewTextureID) {
        float scale = 0.5f;
        ImGui::Image((ImTextureID)(intptr_t)previewTextureID,
                     ImVec2(previewTexWidth * scale, previewTexHeight * scale));
    } else {
        ImGui::Text("No preview available.");
    }

    if (ImGui::Button("Download PDF")) {
        SaveRecipePDF(appState, filenameStr);
        ConvertPDFToPNG(filenameStr + ".pdf", filenameStr);

        if (previewTextureID) {
            glDeleteTextures(1, &previewTextureID);
            previewTextureID = 0;
        }

        previewTextureID = LoadTextureGL((filenameStr + ".png").c_str(), &previewTexWidth, &previewTexHeight);
        ImGui::OpenPopup("Success");
    }

    if (ImGui::BeginPopupModal("Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("PDF has been saved successfully!");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    RenderDisplayWindow(appState);

    ImGui::End();

    ImGui::Begin("Main Menu Controls");

    if (ImGui::Button("Back to Main Menu")) {
        appState.currentPage = Page::MainMenu;
    }

    ImGui::SameLine();

    if (ImGui::Button("Add Recipe to Database")) {
	
	appState.currentPage = Page::RecipeCreate;
    }

    ImGui::End();

}

