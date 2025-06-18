#pragma once
#include <string>
#include <vector>

// Declare shared data
extern std::vector<std::string> recipe_names;
extern std::vector<std::string> ingredients;
extern std::vector<std::string> directions;

// Declare function to load the CSV
void load_recipes(const std::string& filename);
