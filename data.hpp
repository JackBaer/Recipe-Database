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

void clean_all_ingredients_in_recipes();

std::vector<Ingredient> parse_ingredients(const std::string& ingredients_text);
std::string clean_and_format_ingredients(const std::vector<Ingredient>& ingredients); 
std::string clean_recipe_directions(const std::string& input_text);
std::vector<std::string> split_numbered_steps(const std::string& text);
double parse_mixed_fraction(const std::string& str);

void read_recipes_from_csv(const std::string& filename);

void load_recipes(const std::string& filename);

void AppendRecipeToCSV(const std::string& filename,
                       const std::string& name,
                       const std::string& totalTime,
                       const std::string& ingredients,
                       const std::string& directions);

std::string SerializeIngredients(const std::vector<Ingredient>& ingredients);
