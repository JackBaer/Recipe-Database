#pragma once
#include <string>
#include <vector>

struct Ingredient {
    std::string quantity;
    std::string name;
    std::string unit;
};

struct Recipe {
    std::string name;
    std::vector<Ingredient> ingredients;
    std::string directions;
};

// Declare shared data
extern std::vector<std::string> recipe_names;
extern std::vector<Ingredient> ingredients;
extern std::vector<std::string> directions;

extern std::vector<Recipe> recipes;

extern std::vector<std::string> availableUnits;

std::vector<Ingredient> parse_ingredients(const std::string& ingredients_text);
std::string clean_and_format_ingredients(const std::vector<Ingredient>& ingredients); 

void read_recipes_from_csv(const std::string& filename);

void load_recipes(const std::string& filename);
