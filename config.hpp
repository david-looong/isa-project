#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>
#include <string>
#include <map>
#include <unordered_set>

// Enum classes for categorical-type command line switches.
enum class CacheType {dm, sa, fa};
enum class CacheWrite {wb, wt};
enum class CacheEviction {fifo, lru, lfu};
enum class CacheCoherency {none, snoop, dir};
enum class TLBEviction {fifo, lfu, lru};

// Configuration for cache types to take ways exponent into account.
struct CacheTypeConfig {
    CacheType type;
    uint8_t ways_exponent;

    CacheTypeConfig(CacheType t, uint8_t e) {
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
    uint8_t harts = 4;

    // Cycle frequency in Hertz
    uint32_t hz = 100;

    // Cache levels (No cache, L1, L1/L2, L1/L2/L3)
    uint8_t cL = 3;

    // Cache sizes in bytes (L1, L2, L3)
    uint32_t cS1 = 1024;
    uint32_t cS2 = 2048;
    uint32_t cS3 = 4096;

    // Cache block sizes in bytes (L1, L2, L3)
    uint8_t cB1 = 4;
    uint8_t cB2 = 5;
    uint8_t cB3 = 6;

    // # of rows in branch predictor table
    uint8_t bp = 64;

    // Default prediction if branch address is not in table
    uint8_t bpD = 0;

    // # of TLB slots
    uint8_t tlb = 8;

    // Cache types (L1, L2, L3)
    CacheTypeConfig c1 = CacheTypeConfig(CacheType::dm, 0);
    CacheTypeConfig c2 = CacheTypeConfig(CacheType::sa, 3);
    CacheTypeConfig c3 = CacheTypeConfig(CacheType::fa, 0);

    // Cache write policies (L1, L2, L3)
    CacheWrite cW1 = CacheWrite::wt;
    CacheWrite cW2 = CacheWrite::wt;
    CacheWrite cW3 = CacheWrite::wb;

    // Cache eviction policies (L1, L2, L3)
    CacheEviction cE1 = CacheEviction::fifo;
    CacheEviction cE2 = CacheEviction::lru;
    CacheEviction cE3 = CacheEviction::lfu;

    // Cache coherency protocol
    CacheCoherency cC = CacheCoherency::snoop;

    // TLB eviction policy
    TLBEviction tlbE = TLBEviction::lru;
};

void fatal_error(std::string error);
std::map<std::string,std::string> parse_config(std::string config_file);
std::map<std::string,std::string> parse_cli(int argc, char* argv[]);

#endif // CONFIG_HPP