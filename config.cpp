#include "config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <functional>
#include <vector>
#include <algorithm>

std::unordered_map<std::string, std::function<bool(const std::string&)>> switch_table_validator = {
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

std::unordered_map<std::string, std::function<void(Config&, const std::string&, std::string&)>> switch_table_modifier = {
    {"f", skip_change},
    {"i", skip_change},
    {"o", skip_change},
    {"m", change_bool},
    {"a", change_bool},
    {"mem", change_suffix_b},
    {"harts", change_numeric},
    {"hz", change_suffix_d},
    {"cL", change_numeric},
    {"c1", change_cache},
    {"c2", change_cache},
    {"c3", change_cache},
    {"cS1", change_suffix_b},
    {"cS2", change_suffix_b},
    {"cS3", change_suffix_b},
    {"cB1", change_numeric},
    {"cB2", change_numeric},
    {"cB3", change_numeric},
    {"cW1", change_enum_write},
    {"cW2", change_enum_write},
    {"cW3", change_enum_write},
    {"cE1", change_enum_eviction},
    {"cE2", change_enum_eviction},
    {"cE3", change_enum_eviction},
    {"cC", change_enum_coherency},
    {"bp", change_numeric},
    {"bpD", change_numeric},
    {"mmu", change_bool},
    {"tlb", change_numeric},
    {"tlbE", change_enum_eviction}
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
    return (val == "on" || val == "off");
}

bool check_enum_val_eviction(const std::string& val) {
    return (val == "fifo" || val == "lru" || val == "lfu");
}

bool check_enum_val_write(const std::string& val) {
    return (val == "wb" || val == "wt");
}

bool check_enum_val_coherency(const std::string& val) {
    return (val == "none" || val == "snoop" || val == "dir");
}

bool check_cache(const std::string& val) {
    std::vector<std::string> cache_type;
    std::stringstream ss(val);
    std::string w;

    while (std::getline(ss, w, ',')) {
        cache_type.push_back(w);
    }

    if (cache_type[0] == "sa" && cache_type.size() != 2) {
        return false;
    }

    return (cache_type[0] == "dm"
            || (cache_type[0] == "sa" && std::all_of(cache_type[1].begin(), cache_type[1].end(), ::isdigit))
            || cache_type[0] == "fa");
}

void skip_change(Config& config, const std::string& key, std::string& val) {
    return;
}

void change_numeric(Config& config, const std::string& key, std::string& val) {
    if (key == "harts")     config.harts = static_cast<uint32_t>(std::stoul(val));
    else if (key == "cL")   config.cL = static_cast<uint32_t>(std::stoul(val));
    else if (key == "cB1")  config.cB1 = static_cast<uint32_t>(std::stoul(val));
    else if (key == "cB2")  config.cB2 = static_cast<uint32_t>(std::stoul(val));
    else if (key == "cB3")  config.cB3 = static_cast<uint32_t>(std::stoul(val));
    else if (key == "bp")   config.bp = static_cast<uint32_t>(std::stoul(val));
    else if (key == "bpD")  config.bpD = static_cast<uint32_t>(std::stoul(val));
    else if (key == "tlb")  config.tlb = static_cast<uint32_t>(std::stoul(val));

    return;
}

void change_suffix_b(Config& config, const std::string& key, std::string& val) {
    uint32_t mult = 1;
    
    if (val.back() == 'K')      mult = static_cast<uint32_t>(std::pow(1024, 1));
    else if (val.back() == 'M') mult = static_cast<uint32_t>(std::pow(1024, 2));
    else if (val.back() == 'G') mult = static_cast<uint32_t>(std::pow(1024, 3));

    if (val.back() == 'K' || val.back() == 'M' || val.back() == 'G')    val.pop_back();

    if (key == "mem")           config.mem = static_cast<uint32_t>(std::stoul(val)) * mult;
    else if (key == "cS1")      config.cS1 = static_cast<uint32_t>(std::stoul(val)) * mult;
    else if (key == "cS2")      config.cS2 = static_cast<uint32_t>(std::stoul(val)) * mult;
    else if (key == "cS3")      config.cS3 = static_cast<uint32_t>(std::stoul(val)) * mult;
    
    return;
}

void change_suffix_d(Config& config, const std::string& key, std::string& val) {
    uint32_t mult = 1;
    
    if (val.back() == 'K')      mult = static_cast<uint32_t>(std::pow(1000, 1));
    else if (val.back() == 'M') mult = static_cast<uint32_t>(std::pow(1000, 2));
    else if (val.back() == 'G') mult = static_cast<uint32_t>(std::pow(1000, 3));

    if (val.back() == 'K' || val.back() == 'M' || val.back() == 'G')    val.pop_back();

    config.hz = static_cast<uint32_t>(std::stoul(val)) * mult;
    
    return;
}

void change_bool(Config& config, const std::string& key, std::string& val) {
    if (key == "m")         config.m = (val == "on");
    else if (key == "a")    config.a = (val == "on");
    else if (key == "mmu")  config.mmu = (val == "on");

    return;
}

void change_enum_eviction(Config& config, const std::string& key, std::string& val) {
    EvictionPolicy evic_val;

    if (val == "fifo")      evic_val = EvictionPolicy::fifo;
    else if (val == "lru")  evic_val = EvictionPolicy::lru;
    else if (val == "lfu")  evic_val = EvictionPolicy::lfu;

    if (key == "cE1")       config.cE1 = evic_val;
    else if (key == "cE2")  config.cE2 = evic_val;
    else if (key == "cE3")  config.cE3 = evic_val;
    else if (key == "tlbE") config.tlbE = evic_val;

    return;
}

void change_enum_write(Config& config, const std::string& key, std::string& val) {
    WritePolicy write_val;

    if (val == "wt")        write_val = WritePolicy::wt;
    else if (val == "wb")   write_val = WritePolicy::wb;

    if (key == "cW1")       config.cW1 = write_val;
    else if (key == "cW2")  config.cW2 = write_val;
    else if (key == "cW3")  config.cW3 = write_val;

    return;
}

void change_enum_coherency(Config& config, const std::string& key, std::string& val) {
    if (val == "none")          config.cC = CacheCoherency::none;
    else if (val == "snoop")    config.cC = CacheCoherency::snoop;
    else if (val == "dir")      config.cC = CacheCoherency::dir;

    return;
}

void change_cache(Config& config, const std::string& key, std::string& val) {
    std::vector<std::string> cache_type;
    std::stringstream ss(val);
    std::string w;
    CacheTypeConfig cache_config = CacheTypeConfig();

    while (std::getline(ss, w, ',')) {
        cache_type.push_back(w);
    }

    if (cache_type[0] == "dm") {
        cache_config.type = CacheType::dm;
    }
    else if (cache_type[0] == "sa") {
        cache_config.type = CacheType::sa;
        cache_config.ways_exponent = static_cast<uint32_t>(std::stoul(cache_type[1]));
    } else if (cache_type[0] == "fa") {
        cache_config.type = CacheType::fa;
    }

    if (key == "c1")        config.c1 = cache_config;
    else if (key == "c2")   config.c2 = cache_config;
    else if (key == "c3")   config.c3 = cache_config;

    return;
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
            if (switch_table_validator.find(key) == switch_table_validator.end()) {
                fatal_error("Invalid configuration switch.");
            }
        } else {
            ss >> val;
            if (ss.fail()) {
                fatal_error("No value after switch.");
            }

            std::function<bool(const std::string&)> validator = switch_table_validator.find(key)->second;
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

            if (switch_table_validator.find(key) == switch_table_validator.end()
                || key == "f"
                || key == "i"
                || key == "o") {
                fatal_error("Invalid configuration switch.");
            }

            ss >> val;
            if (ss.fail()) {
                fatal_error("No value after switch.");
            }

            std::function<bool(const std::string&)> validator = switch_table_validator.find(key)->second;
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

Config value_conversion(std::map<std::string,std::string> final_map) {
    Config config;
    std::function<void(Config&, const std::string&, std::string&)> change_function;

    for (std::map<std::string,std::string>::iterator it = final_map.begin(); it != final_map.end(); it++) {
        change_function = switch_table_modifier.find(it->first)->second;
        change_function(config, it->first, it->second);
    }
    
    return config;
}