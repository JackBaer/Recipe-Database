#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "pdfExporter.h"

#include <unordered_map>

std::string replace_unicode_fractions(const std::string& input) {
    std::string output = input;
    std::unordered_map<std::string, std::string> fraction_map = {
        {"½", "1/2"}, {"¼", "1/4"}, {"¾", "3/4"},
        {"⅓", "1/3"}, {"⅔", "2/3"}, {"⅛", "1/8"},
        {"⅜", "3/8"}, {"⅝", "5/8"}, {"⅞", "7/8"}
    };
    for (const auto& [unicode, ascii] : fraction_map) {
        size_t pos;
        while ((pos = output.find(unicode)) != std::string::npos) {
            output.replace(pos, unicode.size(), ascii);
        }
    }
    return output;
}

std::vector<std::string> WrapText(HPDF_Page page, const std::string& text, float maxWidth, HPDF_Font font, float fontSize) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string word, line;

    HPDF_Page_SetFontAndSize(page, font, fontSize);

    while (iss >> word) {
        std::string testLine = line.empty() ? word : line + " " + word;
        float textWidth = HPDF_Page_TextWidth(page, testLine.c_str());

        if (textWidth > maxWidth) {
            if (!line.empty()) lines.push_back(line);
            line = word;
        } else {
            line = testLine;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void* user_data) {
    std::cerr << "libharu error: error_no=" << error_no << ", detail_no=" << detail_no << std::endl;
    throw std::runtime_error("libharu error");
}
void SaveRecipePDF(const AppState& appState, const std::string& filename) {
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

        HPDF_Page_SetFontAndSize(page, font, 18);
        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, margin, y, appState.current_recipe.c_str());
        HPDF_Page_EndText(page);
        y -= 30;

        HPDF_Page_SetFontAndSize(page, font, 12);
        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, margin, y, "Ingredients:");
        HPDF_Page_EndText(page);
        y -= 20;

        for (const auto& ing : appState.current_ingredients) {
            std::ostringstream ing_line;
            ing_line << replace_unicode_fractions(ing.quantity) << " "
                     << ing.unit << " " << ing.name;

            auto wrapped = WrapText(page, ing_line.str(), width - 2 * margin - 20, font, 12);
            for (const auto& w : wrapped) {
                HPDF_Page_BeginText(page);
                HPDF_Page_TextOut(page, margin + 20, y, w.c_str());
                HPDF_Page_EndText(page);
                y -= 15;

                if (y < margin) {
                    page = HPDF_AddPage(pdf);
                    HPDF_Page_SetFontAndSize(page, font, 12);
                    y = height - margin;
                }
            }
        }

        HPDF_Page_BeginText(page);
        HPDF_Page_TextOut(page, margin, y, "Directions:");
        HPDF_Page_EndText(page);
        y -= 20;

        std::string cleaned = replace_unicode_fractions(clean_recipe_directions(appState.current_directions));
        std::istringstream dir_stream(cleaned);
        std::string line;
        while (std::getline(dir_stream, line)) {
            auto wrapped = WrapText(page, line, width - 2 * margin - 20, font, 12);
            for (const auto& w : wrapped) {
                HPDF_Page_BeginText(page);
                HPDF_Page_TextOut(page, margin + 20, y, w.c_str());
                HPDF_Page_EndText(page);
                y -= 15;

                if (y < margin) {
                    page = HPDF_AddPage(pdf);
                    HPDF_Page_SetFontAndSize(page, font, 12);
                    y = height - margin;
                }
            }
        }

	std::string pdfFile = filename + ".pdf";
	HPDF_SaveToFile(pdf, pdfFile.c_str());
        std::cout << "PDF saved to: " << pdfFile << std::endl;
    } catch (...) {
        std::cerr << "Failed to write PDF.\n";
    }

    HPDF_Free(pdf);
}
/*
void ConvertPDFToPNG(const std::string& pdfPath, const std::string& outputPrefix) {
    std::string command = "pdftoppm -png -singlefile \"" + pdfPath + "\" \"" + outputPrefix + "\"";
    std::system(command.c_str()); // generates preview.png
}
*/
void ConvertPDFToPNG(const std::string& pdfPath, const std::string& outputPrefix) {
    std::string command = "pdftoppm -png -singlefile \"" + pdfPath + "\" \"" + outputPrefix + "\"";
    std::system(command.c_str());
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

