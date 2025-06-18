#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>                 // C++17

std::vector<std::string> recipe_names;
std::vector<std::string> ingredients;
std::vector<std::string> directions;


// Utility to trim whitespace from both ends
std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) ++start;

    auto end = s.end();
    do {
        --end;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}


// Parse a CSV line accounting for quoted fields
std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> result;
    std::istringstream stream(line);
    std::string field;
    bool inside_quotes = false;
    std::string value;

    for (char c : line) {
        if (c == '"' && (value.empty() || value.back() != '\\')) {
            inside_quotes = !inside_quotes;
            continue;
        }

        if (c == ',' && !inside_quotes) {
            result.push_back(trim(value));
            value.clear();
        } else {
            value += c;
        }
    }
    result.push_back(trim(value)); // add last field
    return result;
}


void load_recipes(const std::string& filename) {
 
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open recipes.csv\n";
    }

    std::string header_line;
    std::getline(file, header_line);
    std::vector<std::string> headers = parse_csv_line(header_line);

    // Find column indices
    int name_idx = -1, ingredients_idx = -1, directions_idx = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "recipe_name") name_idx = i;
        else if (headers[i] == "ingredients") ingredients_idx = i;
        else if (headers[i] == "directions") directions_idx = i;
    }

    if (name_idx == -1 || ingredients_idx == -1 || directions_idx == -1) {
        std::cerr << "One or more required columns not found\n";
    }

    // Vectors to hold the data
    //std::vector<std::string> recipe_names;
    //std::vector<std::string> ingredients;
    //std::vector<std::string> directions;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> fields = parse_csv_line(line);
        if (fields.size() <= std::max({name_idx, ingredients_idx, directions_idx})) continue;

        recipe_names.push_back(fields[name_idx]);
        ingredients.push_back(fields[ingredients_idx]);
        directions.push_back(fields[directions_idx]);
    }

    file.close();
}

/*
int main() {
    std::ifstream file("recipes.csv");
    if (!file.is_open()) {
        std::cerr << "Failed to open recipes.csv\n";
        return 1;
    }

    std::string header_line;
    std::getline(file, header_line);
    std::vector<std::string> headers = parse_csv_line(header_line);

    // Find column indices
    int name_idx = -1, ingredients_idx = -1, directions_idx = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "recipe_name") name_idx = i;
        else if (headers[i] == "ingredients") ingredients_idx = i;
        else if (headers[i] == "directions") directions_idx = i;
    }

    if (name_idx == -1 || ingredients_idx == -1 || directions_idx == -1) {
        std::cerr << "One or more required columns not found\n";
        return 1;
    }

    // Vectors to hold the data
    std::vector<std::string> recipe_names;
    std::vector<std::string> ingredients;
    std::vector<std::string> directions;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> fields = parse_csv_line(line);
        if (fields.size() <= std::max({name_idx, ingredients_idx, directions_idx})) continue;

        recipe_names.push_back(fields[name_idx]);
        ingredients.push_back(fields[ingredients_idx]);
        directions.push_back(fields[directions_idx]);
    }

    file.close();

    // Optional: Print how many recipes were loaded
    std::cout << "Loaded " << recipe_names.size() << " recipes.\n";

    // Example: Print first recipe
    if (!recipe_names.empty()) {
        std::cout << "First Recipe:\n";
        std::cout << "Name: " << recipe_names[0] << "\n";
        std::cout << "Ingredients: " << ingredients[0] << "\n";
        std::cout << "Directions: " << directions[0] << "\n";
    }

    return 0;
}
*/
