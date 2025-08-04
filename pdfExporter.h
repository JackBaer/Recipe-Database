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

GLuint LoadTextureGL(const char* filename, int* out_width, int* out_height);

void SaveRecipePDF(const AppState& appState, const std::string& filename);
void ConvertPDFToPNG(const std::string& pdfPath, const std::string& outputPrefix);

#endif
