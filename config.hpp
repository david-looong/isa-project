/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This header file contains a Config struct, validation functions,
 * modifier functions, and parsing functions that help with configuring the machine.
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <functional>
#include <vector>
#include <algorithm>
#include "error.hpp"

// Enum classes for categorical-type command line switches.
enum class CacheType {dm, sa, fa};
enum class WritePolicy {wb, wt};
enum class EvictionPolicy {fifo, lru, lfu};
enum class CacheCoherency {none, snoop, dir};

// Configuration for cache types to take ways exponent into account.
struct CacheTypeConfig {
    CacheType type;
    uint32_t ways_exponent;

    CacheTypeConfig() {
        ways_exponent = 0;
    };

    CacheTypeConfig(CacheType t, uint32_t e) {
        type = t;
        ways_exponent = e;
    }
};

// Configuration for all virtual machine command line switches.
struct Config {
    // State of RV32M extension
    bool m = true;

    // State of RV32A extension
    bool a = true;

    // MMU state
    bool mmu = true;

    // RAM size
    uint32_t mem = 1048576;

    // # of HARTs
    uint32_t harts = 4;

    // Cycle frequency in Hertz
    uint32_t hz = 100;

    // Cache levels (No cache, L1, L1/L2, L1/L2/L3)
    uint32_t cL = 3;

    // Cache sizes in bytes (L1, L2, L3)
    uint32_t cS1 = 1024;
    uint32_t cS2 = 2048;
    uint32_t cS3 = 4096;

    // Cache block sizes in bytes (L1, L2, L3)
    uint32_t cB1 = 4;
    uint32_t cB2 = 5;
    uint32_t cB3 = 6;

    // # of rows in branch predictor table
    uint32_t bp = 64;

    // Default prediction if branch address is not in table
    uint32_t bpD = 0;

    // # of TLB slots
    uint32_t tlb = 8;

    // Cache types (L1, L2, L3)
    CacheTypeConfig c1 = CacheTypeConfig(CacheType::dm, 0);
    CacheTypeConfig c2 = CacheTypeConfig(CacheType::sa, 3);
    CacheTypeConfig c3 = CacheTypeConfig(CacheType::fa, 0);

    // Cache write policies (L1, L2, L3)
    WritePolicy cW1 = WritePolicy::wt;
    WritePolicy cW2 = WritePolicy::wt;
    WritePolicy cW3 = WritePolicy::wb;

    // Cache eviction policies (L1, L2, L3)
    EvictionPolicy cE1 = EvictionPolicy::fifo;
    EvictionPolicy cE2 = EvictionPolicy::lru;
    EvictionPolicy cE3 = EvictionPolicy::lfu;

    // Cache coherency protocol
    CacheCoherency cC = CacheCoherency::snoop;

    // TLB eviction policy
    EvictionPolicy tlbE = EvictionPolicy::lru;
};

// Validation functions
bool skip_valid(const std::string& val);
bool is_numeric(const std::string& val);
bool is_in_range_1_to_16(const std::string& val);
bool is_in_range_0_to_3(const std::string& val);
bool has_valid_suffix_b(const std::string& val);
bool has_valid_suffix_d(const std::string& val);
bool is_on_or_off(const std::string& val);
bool check_enum_val_eviction(const std::string& val);
bool check_enum_val_write(const std::string& val);
bool check_enum_val_coherency(const std::string& val);
bool check_cache(const std::string& val);

// Modifier functions
void skip_change(Config& config, const std::string& key, std::string& val);
void change_numeric(Config& config, const std::string& key, std::string& val);
void change_suffix_b(Config& config, const std::string& key, std::string& val);
void change_suffix_d(Config& config, const std::string& key, std::string& val);
void change_bool(Config& config, const std::string& key, std::string& val);
void change_enum_eviction(Config& config, const std::string& key, std::string& val);
void change_enum_write(Config& config, const std::string& key, std::string& val);
void change_enum_coherency(Config& config, const std::string& key, std::string& val);
void change_cache(Config& config, const std::string& key, std::string& val);

// Parsing functions
std::map<std::string,std::string> parse_cli(int argc, char* argv[]);
std::map<std::string,std::string> parse_config(std::string config_file);

// Conversion pass function
Config value_conversion(std::map<std::string,std::string> final_map);

#endif // CONFIG_HPP