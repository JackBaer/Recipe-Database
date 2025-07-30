#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include <SDL3/SDL.h>
#include <OpenGL/gl3.h>

#include <hpdf.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "appState.h"
#include "data.hpp"

void GeneratePDF(const std::string& title, const std::vector<Ingredient>& ingredients, const std::string& directions);
void ConvertPDFToPNG(const std::string& pdfPath, const std::string& outputPrefix);
GLuint LoadTextureGL(const char* filename, int* out_width, int* out_height);

void SaveRecipePDF(const AppState& appState);

#endif
