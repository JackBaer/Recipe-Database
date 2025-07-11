#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>                 // C++17
#include <regex>
#include <unordered_map>
#include <cctype>
#include <set>

#include "data.hpp"

std::vector<std::string> recipe_names;
std::vector<std::string> directions;
std::vector<Ingredient> ingredients;

std::vector<Recipe> recipes;

std::vector<std::string> availableUnits;

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

/*
std::vector<Ingredient> parse_ingredients(const std::string& ingredients_text) {
    std::vector<Ingredient> result;
    std::stringstream ss(ingredients_text);
    std::string token;

    std::regex pattern(R"(^\s*([\d¼½¾⅓⅔⅛⅜⅝⅞.\-/\s]+(?:[a-zA-Z]+)?(?:\s+[a-zA-Z]+)?)\s+(.+?)\s*$)");
    std::smatch match;

    while (std::getline(ss, token, ',')) {
        if (std::regex_match(token, match, pattern)) {
            result.push_back({match[1].str(), match[2].str()});
        } else {
            result.push_back({"", token});  // fallback
        }
    }

    return result;
}
*/

const std::set<std::string> known_units = {
    "tsp", "tbsp", "cup", "oz", "g", "gram", "grams", "ml", "l", "lb", "kg", "teaspoon", "tablespoon", "pinch", "dash"
};

std::vector<Ingredient> parse_ingredients(const std::string& ingredients_text) {
    std::vector<Ingredient> result;
    std::stringstream ss(ingredients_text);
    std::string token;

    while (std::getline(ss, token, ',')) {
        Ingredient ing;
        std::istringstream iss(token);
        std::string word;

        // Step 1: Parse quantity (numbers or unicode fractions)
        std::string quantity_part;
        while (iss >> word) {
            if (std::regex_match(word, std::regex(R"([\d¼½¾⅓⅔⅛⅜⅝⅞/\.]+)"))) {
                if (!quantity_part.empty())
                    quantity_part += " ";
                quantity_part += word;
            } else {
                break; // stop at first non-number
            }
        }
        ing.quantity = quantity_part;

        // Step 2: Parse unit if it's a known unit
        std::string unit_candidate = word;
        std::transform(unit_candidate.begin(), unit_candidate.end(), unit_candidate.begin(), ::tolower);
        if (known_units.count(unit_candidate)) {
            ing.unit = unit_candidate;
        } else {
            // Not a valid unit, treat as part of name
            if (!unit_candidate.empty())
                ing.name += unit_candidate + " ";
        }

        // Step 3: Append remaining words to name
        std::string rest;
        while (iss >> word) {
            ing.name += word + " ";
        }

        // Trim trailing space from name
        ing.name.erase(ing.name.find_last_not_of(" \t\n\r\f\v") + 1);

        result.push_back(ing);
    }

    return result;
}


/*
std::vector<Ingredient> parse_ingredients(const std::string& ingredients_text) {
    std::vector<Ingredient> result;
    std::stringstream ss(ingredients_text);
    std::string token;

    std::regex pattern(R"(^\s*([\d¼½¾⅓⅔⅛⅜⅝⅞\.\-/\s]+)([a-zA-Z]+)?\s+(.*)$)");
    std::smatch match;

    while (std::getline(ss, token, ',')) {
        std::string trimmed = token;
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), trimmed.end());

        if (std::regex_match(trimmed, match, pattern)) {
            std::string qty = match[1].str();
            std::string unit = match[2].str();  // optional
            std::string name = match[3].str();

            result.push_back({qty, unit, name});
        } else {
            // fallback: store full text in name
            result.push_back({"", "", token});
        }
    }

    return result;
}
*/

void read_recipes_from_csv(const std::string& filename) {
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
        if (fields.size() <= std::max({name_idx, ingredients_idx, directions_idx})) {
            std::cerr << "Skipping malformed row with only " << fields.size() << " fields\n";
            continue;
        }

        Recipe r;
        r.name = fields[name_idx];
        r.directions = fields[directions_idx];

        try {
            r.ingredients = parse_ingredients(fields[ingredients_idx]);
        } catch (const std::regex_error& e) {
            std::cerr << "Regex error while parsing ingredients: " << e.what() << "\n";
            continue;
        }

        recipes.push_back(r);  // global vector
    }

    std::set<std::string> unit_set;
 
 /*   
    for (const auto& recipe : recipes) {
	    for (const auto& ing : recipe.ingredients) {
		if (!ing.unit.empty()) {
		    std::string unit = ing.unit;
		    std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
		    unit_set.insert(unit);
		}
	    }
    }
*/
    for (const auto& recipe : recipes) {
        for (const auto& ing : recipe.ingredients) {
            if (!ing.unit.empty())
                unit_set.insert(ing.unit);
        }
    }

    std::vector<std::string> unitsList(unit_set.begin(), unit_set.end());

    availableUnits = unitsList;

    file.close();
}

std::string clean_and_format_ingredients(const std::vector<Ingredient>& ingredients) {
    std::unordered_map<std::string, std::string> unit_map = {
        {"T", "tbsp"},
        {"Tbsp", "tbsp"},
        {"TBS", "tbsp"},
        {"Tablespoon", "tbsp"},
        {"tablespoons", "tbsp"},
        {"tablespoon", "tbsp"},
        {"t", "tsp"},
        {"tsp", "tsp"},
        {"Teaspoon", "tsp"},
        {"teaspoons", "tsp"},
        {"teaspoon", "tsp"},
        {"C", "cup"},
        {"Cup", "cup"},
        {"cups", "cup"},
        {"c", "cup"},
        {"oz", "oz"},
        {"ounce", "oz"},
        {"ounces", "oz"},
        {"ml", "ml"},
        {"l", "l"},
        {"g", "g"},
        {"kg", "kg"}
    };

    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    };

    std::ostringstream oss;

    for (const auto& ing : ingredients) {
        std::string qty = ing.quantity;
        std::string name = ing.name;

        trim(qty);
        trim(name);

        // Normalize units in qty using regex
        for (const auto& [key, replacement] : unit_map) {
            std::regex pattern("\\b" + key + "\\b", std::regex_constants::icase);
            qty = std::regex_replace(qty, pattern, replacement);
        }

        // Combine cleaned parts
        if (!qty.empty() && !name.empty())
            oss << qty << " " << name << "\n";
        else if (!name.empty())
            oss << name << "\n";  // If quantity is missing, just show name
    }

    return oss.str();
}
