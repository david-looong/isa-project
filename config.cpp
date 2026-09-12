#include "config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <functional>
#include <vector>

std::unordered_map<std::string, std::function<bool(const std::string&)>> switch_table = {
    {"f", skip_valid},
    {"i", skip_valid},
    {"o", skip_valid},
    {"m", is_on_or_off},
    {"a", is_on_or_off},
    {"mem", has_valid_suffix_b},
    {"harts", is_in_range_1_to_16},
    {"hz", has_valid_suffix_d},
    {"cL", is_in_range_0_to_3},
    {"c1", check_cache},
    {"c2", check_cache},
    {"c3", check_cache},
    {"cS1", has_valid_suffix_b},
    {"cS2", has_valid_suffix_b},
    {"cS3", has_valid_suffix_b},
    {"cB1", is_numeric},
    {"cB2", is_numeric},
    {"cB3", is_numeric},
    {"cW1", check_enum_val_write},
    {"cW2", check_enum_val_write},
    {"cW3", check_enum_val_write},
    {"cE1", check_enum_val_eviction},
    {"cE2", check_enum_val_eviction},
    {"cE3", check_enum_val_eviction},
    {"cC", check_enum_val_coherency},
    {"bp", is_numeric},
    {"bpD", is_in_range_0_to_3},
    {"mmu", is_on_or_off},
    {"tlb", is_numeric},
    {"tlbE", check_enum_val_eviction}
};

// Handles fatal errors by printing the error message and exiting the program.
void fatal_error(std::string error) {
    std::cerr << "Fatal error: " << error << std::endl;
    std::exit(1);
}

bool skip_valid(const std::string& val) {
    return true;
}

bool is_numeric(const std::string& val) {
    return (std::all_of(val.begin(), val.end(), ::isdigit));
}

bool is_in_range_1_to_16(const std::string& val) {
    if (!std::all_of(val.begin(), val.end(), ::isdigit)) {
        return false;
    }
    return (std::stoi(val) <= 16 && std::stoi(val) >= 1);
}

bool is_in_range_0_to_3(const std::string& val) {
    if (!std::all_of(val.begin(), val.end(), ::isdigit)) {
        return false;
    }
    return (std::stoi(val) <= 3 && std::stoi(val) >= 0);
}

bool has_valid_suffix_b(const std::string& val) {
    if (val.back() == 'K' || val.back() == 'M' || val.back() == 'G') {
        return true;
    }
    return (std::all_of(val.begin(), val.end(), ::isdigit));
}

bool has_valid_suffix_d(const std::string& val) {
    if (val.back() == 'K' || val.back() == 'M' || val.back() == 'G') {
        return true;
    }
    return (std::all_of(val.begin(), val.end(), ::isdigit));
}

bool is_on_or_off(const std::string& val) {
    return (val.compare("on") == 0 || val.compare("off") == 0);
}

bool check_enum_val_eviction(const std::string& val) {
    return (val.compare("fifo") == 0 || val.compare("lru") == 0 || val.compare("lfu") == 0);
}

bool check_enum_val_write(const std::string& val) {
    return (val.compare("wb") == 0 || val.compare("wt") == 0);
}

bool check_enum_val_coherency(const std::string& val) {
    return (val.compare("none") == 0 || val.compare("snoop") == 0 || val.compare("dir") == 0);
}

bool check_cache(const std::string& val) {
    std::vector<std::string> cache_type;
    std::stringstream ss(val);
    std::string w;

    while (std::getline(ss, w, ',')) {
        cache_type.push_back(w);
    }

    return (cache_type[0].compare("dm") == 0 || cache_type[0].compare("sa") == 0 || cache_type[0].compare("fa") == 0);
}

// Parses command-line arguments and returns a map of configuration switches and their corresponding values.
std::map<std::string,std::string> parse_cli(int argc, char* argv[]) {
    std::string key, val, exe;
    std::map<std::string,std::string> tmp_map;

    for (int i = 1; i < argc; i++) {
        std::stringstream ss(argv[i]);
        
        if (i % 2 != 0) {
            ss >> key;
            key.erase(0, 1);
            if (switch_table.find(key) == switch_table.end()) {
                fatal_error("Invalid configuration switch.");
            }
        } else {
            ss >> val;
            if (ss.fail()) {
                fatal_error("No value after switch.");
            }

            std::function<bool(const std::string&)> validator = switch_table.find(key)->second;
            if (validator(val)) {
                tmp_map[key] = val;
            } else {
                fatal_error("Invalid switch value.");
            }
        }
    }

    return tmp_map;
}

// Parses a configuration file and returns a map of configuration switches and their corresponding values.
std::map<std::string,std::string> parse_config(std::string config_file) {
    std::ifstream fin;
    std::string line, key, val;
    std::map<std::string,std::string> config_map;
    size_t hashtag_pos;

    fin.open(config_file);
    if (!fin.is_open()) {
        fatal_error("Unable to open config file.");
    }

    while (getline(fin, line)) {
        hashtag_pos = line.find('#');
        if (hashtag_pos != std::string::npos) {
            line.resize(hashtag_pos);
        }

        if (line.length() > 0) {
            std::stringstream ss(line);
            ss >> key;

            if (switch_table.find(key) == switch_table.end()) {
                fatal_error("Invalid configuration switch.");
            }

            ss >> val;
            if (ss.fail()) {
                fatal_error("No value after switch.");
            }

            std::function<bool(const std::string&)> validator = switch_table.find(key)->second;
            if (validator(val)) {
                config_map[key] = val;
            } else {
                fatal_error("Invalid switch value.");
            }
        }
    }

    fin.close();
    return config_map;
}
