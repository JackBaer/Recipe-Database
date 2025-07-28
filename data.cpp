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

std::vector<Recipe> recipes;
std::vector<std::string> availableUnits;

std::string normalize_fractions(const std::string& input) {
    static const std::unordered_map<char, std::string> fraction_map = {
        { L'¼', "1/4" }, { L'½', "1/2" }, { L'¾', "3/4" },
        { L'⅓', "1/3" }, { L'⅔', "2/3" },
        { L'⅛', "1/8" }, { L'⅜', "3/8" }, { L'⅝', "5/8" }, { L'⅞', "7/8" }
    };

    std::ostringstream oss;
    for (char c : input) {
        auto it = fraction_map.find(c);
        if (it != fraction_map.end()) {
            oss << it->second;
        } else {
            oss << c;
        }
    }

    return oss.str();
}

const std::unordered_map<std::string, std::string> unit_map = {
    {"T", "tbsp"}, {"Tbsp", "tbsp"}, {"TBS", "tbsp"},
    {"Tablespoon", "tbsp"}, {"tablespoons", "tbsp"}, {"tablespoon", "tbsp"},
    {"t", "tsp"}, {"tsp", "tsp"}, {"Teaspoon", "tsp"}, {"teaspoons", "tsp"}, {"teaspoon", "tsp"},
    {"C", "cup"}, {"Cup", "cup"}, {"cups", "cup"}, {"c", "cup"},
    {"oz", "oz"}, {"ounce", "oz"}, {"ounces", "oz"},
    {"ml", "mL"}, {"l", "L"}, {"g", "g"}, {"kg", "kg"}, {"lbs", "lbs"}
};

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

	std::string unit_candidate = word;
	std::transform(unit_candidate.begin(), unit_candidate.end(), unit_candidate.begin(), ::tolower);
	ing.unit = unit_candidate;

        // Step 2: Append remaining words to name
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

void read_recipes_from_csv(const std::string& filename) {
    // Open File
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    // Read and parse header
    std::string header_line = read_csv_record(file);
    std::vector<std::string> headers = parse_csv_line(header_line);

    int name_idx = -1, ingredients_idx = -1, directions_idx = -1, time_idx = -1;

    // Catch all desired rows, and assign the corresponding id values
    for (size_t i = 0; i < headers.size(); ++i) {
        if (headers[i] == "recipe_name") name_idx = i;
        else if (headers[i] == "ingredients") ingredients_idx = i;
        else if (headers[i] == "directions") directions_idx = i;
	else if (headers[i] == "total_time") time_idx = i;
    }

    // Make sure all columns were found in data
    if (name_idx == -1 || ingredients_idx == -1 || directions_idx == -1 || time_idx == -1) {
        std::cerr << "Required columns not found\n";
        return;
    }

    // Read through file, ensuring every row is complete
    while (file) {
        std::string record = read_csv_record(file);
        if (record.empty()) continue;

        std::vector<std::string> fields = parse_csv_line(record);
        if (fields.size() <= std::max({name_idx, ingredients_idx, directions_idx, time_idx})) {
            std::cerr << "Skipping malformed row with only " << fields.size() << " fields\n";
            continue;
        }

	// Build recipe with data
        Recipe r;
        r.name = fields[name_idx];
        r.directions = fields[directions_idx];
	r.time = fields[time_idx];

	// Make sure all ingredients get parsed properly
        try {
            r.ingredients = parse_ingredients(fields[ingredients_idx]);
        } catch (const std::regex_error& e) {
            std::cerr << "Regex error while parsing ingredients: " << e.what() << "\n";
            continue;
        }

	// Push recipe to global list
        recipes.push_back(r); 
    }

    // Hard code units to be used in drop-down selection
    availableUnits.push_back(" "); // Option for no units
    availableUnits.push_back("tsp");
    availableUnits.push_back("tbsp");    
    availableUnits.push_back("cup");
    availableUnits.push_back("oz");    
    availableUnits.push_back("g");
    availableUnits.push_back("mL");    
    availableUnits.push_back("L");
    availableUnits.push_back("lbs");    

    file.close();
}

/*
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
        {"ml", "mL"},
        {"l", "L"},
        {"g", "g"},
        {"kg", "kg"}
    };

    // Trim string of ingredients
    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    };

    std::ostringstream oss;

    // Standardize ingredients
    for (const auto& ing : ingredients) {
        std::string qty = ing.quantity;
	std::string unit = ing.unit;
	std::string name = ing.name;

        trim(qty);
	trim(unit);
	trim(name);

        // Normalize units in qty using regex
        for (const auto& [key, replacement] : unit_map) {
            std::regex pattern("\\b" + key + "\\b", std::regex_constants::icase);
            qty = std::regex_replace(qty, pattern, replacement);
        }

        // Combine cleaned parts
        if (!qty.empty() && !unit.empty() && !name.empty())
            oss << qty << " " << unit << " " << name << "\n";
	else if (!name.empty())
		oss << name << "\n";  // If quantity or unit is missing, just show name
				  
    }

    return oss.str();
}
*/

std::string clean_and_format_ingredients(const std::vector<Ingredient>& ingredients) {
    std::unordered_map<std::string, std::string> unit_map = {
        {"T", "tbsp"}, {"Tbsp", "tbsp"}, {"TBS", "tbsp"}, {"Tablespoon", "tbsp"},
        {"tablespoons", "tbsp"}, {"tablespoon", "tbsp"}, {"t", "tsp"},
        {"tsp", "tsp"}, {"Teaspoon", "tsp"}, {"teaspoons", "tsp"}, {"teaspoon", "tsp"},
        {"C", "cup"}, {"Cup", "cup"}, {"cups", "cup"}, {"c", "cup"},
        {"oz", "oz"}, {"ounce", "oz"}, {"ounces", "oz"},
        {"ml", "mL"}, {"l", "L"}, {"g", "g"}, {"kg", "kg"}
    };

    // ASCII to Unicode fraction map
    std::unordered_map<std::string, std::string> ascii_to_unicode = {
        {"1/4", "¼"}, {"1/2", "½"}, {"3/4", "¾"},
        {"1/3", "⅓"}, {"2/3", "⅔"},
        {"1/5", "⅕"}, {"2/5", "⅖"}, {"3/5", "⅗"}, {"4/5", "⅘"},
        {"1/6", "⅙"}, {"5/6", "⅚"},
        {"1/8", "⅛"}, {"3/8", "⅜"}, {"5/8", "⅝"}, {"7/8", "⅞"}
    };

    // Trim helper
    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    };

    // Convert "1 1/2" or "1/2" to "1½" or "½"
    auto convert_to_unicode_fractions = [&](std::string qty) -> std::string {
        std::regex mixed_number_pattern(R"((\b\d+)\s+(\d/\d)\b)");
        std::smatch match;

        // Replace mixed numbers first (e.g., 1 1/2 → 1½)
        while (std::regex_search(qty, match, mixed_number_pattern)) {
            std::string whole = match[1];
            std::string frac = match[2];
            auto it = ascii_to_unicode.find(frac);
            if (it != ascii_to_unicode.end()) {
                qty.replace(match.position(0), match.length(0), whole + it->second);
            } else {
                break;
            }
        }

        // Replace standalone fractions (e.g., 1/2 → ½)
        for (const auto& [ascii_frac, unicode_frac] : ascii_to_unicode) {
            std::regex standalone_frac(R"(\b)" + ascii_frac + R"(\b)");
            qty = std::regex_replace(qty, standalone_frac, unicode_frac);
        }

        return qty;
    };

    std::ostringstream oss;

    for (const auto& ing : ingredients) {
        std::string qty = ing.quantity;
        std::string unit = ing.unit;
        std::string name = ing.name;

        trim(qty);
        trim(unit);
        trim(name);

        qty = convert_to_unicode_fractions(qty);

        for (const auto& [key, replacement] : unit_map) {
            std::regex pattern("\\b" + key + "\\b", std::regex_constants::icase);
            qty = std::regex_replace(qty, pattern, replacement);
        }

        if (!qty.empty() && !unit.empty() && !name.empty())
            oss << qty << " " << unit << " " << name << "\n";
        else if (!name.empty())
            oss << name << "\n";
    }

    return oss.str();
}

std::string clean_recipe_directions(const std::string& input_text) {
    // Step 1: Trim leading/trailing whitespace
    std::string text = input_text;
    text.erase(0, text.find_first_not_of(" \t\n\r"));
    text.erase(text.find_last_not_of(" \t\n\r") + 1);

    // Step 2: Remove trailing empty lines or signature lines
    std::istringstream iss(text);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(iss, line)) {
        line.erase(0, line.find_first_not_of(" \t\r"));
        line.erase(line.find_last_not_of(" \t\r") + 1);
        lines.push_back(line);
    }

    while (!lines.empty()) {
        const std::string& last = lines.back();
        if (last.empty() || (last.length() <= 30 && !std::regex_search(last, std::regex(R"([.!?])")))) {
            lines.pop_back();
        } else {
            break;
        }
    }

    // Step 3: Recombine lines into one string
    std::string cleaned_text;
    for (const auto& l : lines) {
        cleaned_text += l + " ";
    }

    // Step 4: Split into sentences
    std::regex sentence_splitter(R"(([^.!?]+[.!?]))");
    std::sregex_iterator it(cleaned_text.begin(), cleaned_text.end(), sentence_splitter);
    std::sregex_iterator end;

    // Step 5: Number each sentence
    std::ostringstream result;
    int step_num = 1;
    for (; it != end; ++it) {
        std::string sentence = it->str();
        sentence.erase(0, sentence.find_first_not_of(" \t\n\r"));
        sentence.erase(sentence.find_last_not_of(" \t\n\r") + 1);

        if (!sentence.empty()) {
            result << step_num++ << ". " << sentence << "\n";
        }
    }

    return result.str();
}

std::vector<std::string> split_numbered_steps(const std::string& text) {
    std::vector<std::string> steps;

    // Match: digit(s), period, space (e.g. "1. ")
    std::regex step_splitter(R"((\d+\.\s))");
    std::sregex_token_iterator it(text.begin(), text.end(), step_splitter, {-1, 0});
    std::sregex_token_iterator end;

    std::string current_step;
    for (; it != end; ++it) {
        std::string token = it->str();

        if (std::regex_match(token, step_splitter)) {
            if (!current_step.empty()) {
                steps.push_back(current_step);
            }
            current_step = token;  // Start new step
        } else {
            current_step += token;
        }
    }

    if (!current_step.empty()) {
        steps.push_back(current_step);
    }

    // Trim whitespace
    for (auto& step : steps) {
        step.erase(0, step.find_first_not_of(" \t\n\r"));
        step.erase(step.find_last_not_of(" \t\n\r") + 1);
    }

    return steps;
}


void clean_all_ingredients_in_recipes() {
    std::unordered_map<std::string, std::string> unit_map = {
        {"T", "tbsp"}, {"Tbsp", "tbsp"}, {"TBS", "tbsp"}, {"Tablespoon", "tbsp"},
        {"tablespoons", "tbsp"}, {"tablespoon", "tbsp"}, {"t", "tsp"},
        {"tsp", "tsp"}, {"Teaspoon", "tsp"}, {"teaspoons", "tsp"}, {"teaspoon", "tsp"},
        {"C", "cup"}, {"Cup", "cup"}, {"cups", "cup"}, {"c", "cup"},
        {"oz", "oz"}, {"ounce", "oz"}, {"ounces", "oz"},
        {"ml", "mL"}, {"l", "L"}, {"g", "g"}, {"kg", "kg"}
    };

    std::unordered_map<std::string, std::string> ascii_to_unicode = {
        {"1/4", "¼"}, {"1/2", "½"}, {"3/4", "¾"},
        {"1/3", "⅓"}, {"2/3", "⅔"},
        {"1/5", "⅕"}, {"2/5", "⅖"}, {"3/5", "⅗"}, {"4/5", "⅘"},
        {"1/6", "⅙"}, {"5/6", "⅚"},
        {"1/8", "⅛"}, {"3/8", "⅜"}, {"5/8", "⅝"}, {"7/8", "⅞"}
    };

    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    };

    auto convert_to_unicode_fractions = [&](std::string& qty) {
        std::regex mixed_number_pattern(R"((\b\d+)\s+(\d/\d)\b)");
        std::smatch match;

        // Convert mixed numbers (e.g., "1 1/2" → "1½")
        while (std::regex_search(qty, match, mixed_number_pattern)) {
            std::string whole = match[1];
            std::string frac = match[2];
            auto it = ascii_to_unicode.find(frac);
            if (it != ascii_to_unicode.end()) {
                qty.replace(match.position(0), match.length(0), whole + it->second);
            } else {
                break;
            }
        }

        // Convert standalone fractions (e.g., "1/2" → "½")
        for (const auto& [ascii_frac, unicode_frac] : ascii_to_unicode) {
            std::regex standalone_frac(R"(\b)" + ascii_frac + R"(\b)");
            qty = std::regex_replace(qty, standalone_frac, unicode_frac);
        }
    };

    for (Recipe& recipe : recipes) {
        for (Ingredient& ing : recipe.ingredients) {
            trim(ing.quantity);
            trim(ing.unit);
            trim(ing.name);

            convert_to_unicode_fractions(ing.quantity);

            for (const auto& [key, replacement] : unit_map) {
                std::regex pattern("\\b" + key + "\\b", std::regex_constants::icase);
                ing.unit = std::regex_replace(ing.unit, pattern, replacement);
            }
        }
    }
}

