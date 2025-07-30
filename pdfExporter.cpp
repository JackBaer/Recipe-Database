#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "pdfExporter.h"

void GeneratePDF(const std::string& title, const std::vector<Ingredient>& ingredients, const std::string& directions) {
    HPDF_Doc pdf = HPDF_New(NULL, NULL);
    if (!pdf) return;

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", NULL), 12);

    float y = HPDF_Page_GetHeight(page) - 40;

    HPDF_Page_BeginText(page);
    HPDF_Page_TextOut(page, 50, y, title.c_str());
    y -= 20;

    for (const auto& ing : ingredients) {
        std::string line = ing.quantity + " " + ing.unit + " " + ing.name;
        HPDF_Page_TextOut(page, 50, y, line.c_str());
        y -= 15;
    }

    y -= 20;
    HPDF_Page_TextOut(page, 50, y, "Directions:");
    y -= 15;

    std::istringstream stream(directions);
    std::string step;
    while (std::getline(stream, step)) {
        HPDF_Page_TextOut(page, 50, y, step.c_str());
        y -= 15;
    }

    HPDF_Page_EndText(page);

    HPDF_SaveToFile(pdf, "recipe_export.pdf");
    HPDF_Free(pdf);
}


void ConvertPDFToPNG(const std::string& pdfPath, const std::string& outputPrefix) {
    std::string command = "pdftoppm -png -singlefile \"" + pdfPath + "\" \"" + outputPrefix + "\"";
    std::system(command.c_str()); // generates preview.png
}

GLuint LoadTextureGL(const char* filename, int* out_width, int* out_height) {
    int w, h, channels;
    unsigned char* data = stbi_load(filename, &w, &h, &channels, 4);
    if (!data)
        return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    if (out_width) *out_width = w;
    if (out_height) *out_height = h;
    return tex;
}


void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void* user_data) {
    std::cerr << "libharu error: error_no=" << error_no << ", detail_no=" << detail_no << std::endl;
    throw std::runtime_error("libharu error");
}

void SaveRecipePDF(const AppState& appState) {
    HPDF_Doc pdf = HPDF_New(error_handler, nullptr);
    if (!pdf) {
        std::cerr << "Failed to create PDF object.\n";
        return;
    }

    try {
        HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

        HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", nullptr);
        float width = HPDF_Page_GetWidth(page);
        float height = HPDF_Page_GetHeight(page);
        float margin = 50;
        float y = height - margin;

        // Title
        HPDF_Page_SetFontAndSize(page, font, 18);
        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, margin, y, appState.current_recipe.c_str());
        HPDF_Page_EndText(page);
        y -= 30;

        // Ingredients
        HPDF_Page_SetFontAndSize(page, font, 12);
        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, margin, y, "Ingredients:");
        HPDF_Page_EndText(page);
        y -= 20;

        for (const auto& ing : appState.current_ingredients) {
            std::ostringstream ing_line;
            ing_line << ing.quantity << " " << ing.unit << " " << ing.name;

            HPDF_Page_BeginText(page);
            HPDF_Page_TextOut(page, margin + 20, y, ing_line.str().c_str());
            HPDF_Page_EndText(page);
            y -= 15;

            if (y < margin) {
                page = HPDF_AddPage(pdf);
                HPDF_Page_SetFontAndSize(page, font, 12);
                y = height - margin;
            }
        }

        // Directions
        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, margin, y, "Directions:");
        HPDF_Page_EndText(page);
        y -= 20;

	std::string cleaned_directions = clean_recipe_directions(appState.current_directions);        
	
	std::istringstream dir_stream(cleaned_directions);
        std::string line;
        while (std::getline(dir_stream, line)) {
            HPDF_Page_BeginText(page);
            HPDF_Page_TextOut(page, margin + 20, y, line.c_str());
            HPDF_Page_EndText(page);
            y -= 15;

            if (y < margin) {
                page = HPDF_AddPage(pdf);
                HPDF_Page_SetFontAndSize(page, font, 12);
                y = height - margin;
            }
        }

        std::string filename = "recipe_export.pdf";
        HPDF_SaveToFile(pdf, filename.c_str());
        std::cout << "PDF saved to: " << filename << std::endl;

    } catch (...) {
        std::cerr << "Failed to write PDF.\n";
    }

    HPDF_Free(pdf);
}
