#include "config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

std::unordered_set<std::string> valid_switches = {
    "f",
    "i",
    "o",
    "m",
    "a",
    "mem",
    "harts",
    "hz",
    "cL",
    "c1",
    "c2",
    "c3",
    "cS1",
    "cS2",
    "cS3",
    "cB1",
    "cB2",
    "cB3",
    "cW1",
    "cW2",
    "cW3",
    "cE1",
    "cE2",
    "cE3",
    "cC",
    "bp",
    "bpD",
    "mmu",
    "tlb",
    "tlbE"
};

void fatal_error(std::string error) {
    std::cerr << "Fatal error: " << error << std::endl;
    std::exit(1);
}

std::map<std::string,std::string> parse_cli(int argc, char* argv[]) {
    std::string key, val, exe;
    std::map<std::string,std::string> tmp_map;

    for (int i = 1; i < argc; i++) {
        std::stringstream ss(argv[i]);
        
        if (i % 2 != 0) {
            ss >> key;
            key.erase(0, 1);
            if (valid_switches.find(key) == valid_switches.end()) {
                fatal_error("Invalid configuration switch.");
            }
        } else {
            ss >> val;
            tmp_map[key] = val;
        }
    }

    return tmp_map;
}

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
        std::cout << line << std::endl;

        hashtag_pos = line.find('#');
        if (hashtag_pos != std::string::npos) {
            line.resize(hashtag_pos);
        }

        if (line.length() > 0) {
            std::stringstream ss(line);
            ss >> key >> val;

            if (valid_switches.find(key) == valid_switches.end()) {
                fatal_error("Invalid configuration switch.");
            }

            config_map[key] = val;
        }
    }

    fin.close();
    return config_map;
}
