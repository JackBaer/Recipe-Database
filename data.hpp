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
    std::string time;
};

// Declare shared data
extern std::vector<Recipe> recipes;
extern std::vector<std::string> availableUnits;

std::vector<Ingredient> parse_ingredients(const std::string& ingredients_text);
std::string clean_and_format_ingredients(const std::vector<Ingredient>& ingredients); 
std::string clean_recipe_directions(const std::string& input_text);
std::vector<std::string> split_numbered_steps(const std::string& text);

void read_recipes_from_csv(const std::string& filename);

void load_recipes(const std::string& filename);
