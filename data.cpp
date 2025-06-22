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


std::string read_csv_record(std::ifstream& file) {
    std::string line, record;
    bool in_quotes = false;

    while (std::getline(file, line)) {
        if (!record.empty()) record += "\n";
        record += line;

        int quote_count = 0;
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    ++i; // escaped quote
                } else {
                    in_quotes = !in_quotes;
                }
            }
        }

        if (!in_quotes) break; // full record captured
    }

    return record;
}

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> result;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"'; // escaped quote
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            result.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    result.push_back(field);
    return result;
}

void load_recipes(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    // Read and parse header
    std::string header_line = read_csv_record(file);
    std::vector<std::string> headers = parse_csv_line(header_line);

    int name_idx = -1, ingredients_idx = -1, directions_idx = -1;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "recipe_name") name_idx = i;
        else if (headers[i] == "ingredients") ingredients_idx = i;
        else if (headers[i] == "directions") directions_idx = i;
    }

    if (name_idx == -1 || ingredients_idx == -1 || directions_idx == -1) {
        std::cerr << "Required columns not found\n";
        return;
    }

    while (file) {
        std::string record = read_csv_record(file);
        if (record.empty()) continue;

        std::vector<std::string> fields = parse_csv_line(record);
        if (fields.size() <= std::max({name_idx, ingredients_idx, directions_idx})) continue;

        recipe_names.push_back(fields[name_idx]);
        ingredients.push_back(fields[ingredients_idx]);
        directions.push_back(fields[directions_idx]);
    }

    file.close();
}

/*
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
*/
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
