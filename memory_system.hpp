/* Name: David Long
 * Date: November 15, 2026
 * Synopsis: This header file contains the entirety of the memory system. It includes
 * the following: several enum classes to classify parking reasons, access types, and
 * atomic ops; a struct to hold contents of a memory operation; a struct to hold all
 * cache config settings; and a memory system class where the memory seam and caches live.
*/

#ifndef MEMORY_SYSTEM_HPP
#define MEMORY_SYSTEM_HPP

#include <cstdint>
#include <memory>
#include "cache.hpp"

enum class ParkReason { Running, System, AmoStore, Load, Fetch, IllegalInstruction, Access };
enum class AccessType { Fetch, Load, Store, LR, SC, Amo };
enum class AtomicOps { Swap, Add, And, Or, Xor, Max, MaxU, Min, MinU };

struct MemorySystemResult {
    uint32_t data = 0;
    uint32_t latency = 0;
    ParkReason pr = ParkReason::Running;
};

struct Settings {
    uint32_t cL = 0;
    uint32_t harts = 1;
    CacheTypeConfig c1_type = CacheTypeConfig(CacheType::dm, 0);
    CacheTypeConfig c2_type = CacheTypeConfig(CacheType::sa, 3);
    CacheTypeConfig c3_type = CacheTypeConfig(CacheType::fa, 0);
    uint32_t cS1 = 1024;
    uint32_t cS2 = 2048;
    uint32_t cS3 = 4096;
    uint32_t cB1 = 4;
    uint32_t cB2 = 5;
    uint32_t cB3 = 6;
    WritePolicy cW1 = WritePolicy::wt;
    WritePolicy cW2 = WritePolicy::wt;
    WritePolicy cW3 = WritePolicy::wb;
    EvictionPolicy cE1 = EvictionPolicy::fifo;
    EvictionPolicy cE2 = EvictionPolicy::lru;
    EvictionPolicy cE3 = EvictionPolicy::lfu;
    uint32_t L1 = 1;
    uint32_t L2 = 4;
    uint32_t L3 = 12;
    uint32_t RAM = 50;
};

class MemorySystem {
    private:
        friend class Machine;
        uint32_t mem_size;
        std::unique_ptr<uint8_t[]> ram;
        Settings s;
        std::vector<Cache> L1;
        std::vector<Cache> L2;
        std::optional<Cache> L3;
        uint32_t read(uint32_t addr, uint8_t size);
        void write(uint32_t addr, uint8_t size, uint32_t value);
        std::vector<uint8_t> ram_range_read(uint64_t paddr, uint32_t len);
        void ram_range_write(uint64_t paddr, const std::vector<uint8_t> &bytes);
        std::vector<uint8_t> range_read(uint32_t hartid, uint8_t level_num, uint64_t paddr, uint32_t len, uint64_t &deepest);
        void range_write(uint32_t hartid, uint8_t level_num, uint64_t paddr, const std::vector<uint8_t> &bytes, uint64_t &deepest);
        // L3: one shared cache, plus its mutex (if -cL 3)
        // bus mutex, or the directory (per -cC)
        // per-HART state, indexed by HART id:
        //      L1, L2, TLB, LR/SC reservation
        // the "busy until tick" counter
    public:
        MemorySystem(uint32_t mem_size, Settings settings);
        MemorySystemResult access(uint32_t vaddr, uint8_t size, AccessType type, [[maybe_unused]] uint32_t mhartid, [[maybe_unused]] uint32_t satp, uint32_t store_val, [[maybe_unused]] uint32_t amo_operand, [[maybe_unused]] AtomicOps op);
        uint32_t get_memory_size();
        Cache& get_cache_level(uint32_t hartid, uint32_t level_num);
};


#endif // MEMORY_SYSTEM_HPP