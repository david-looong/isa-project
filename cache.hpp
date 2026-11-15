/* Name: David Long
 * Date: November 15, 2026
 * Synopsis: This header file contains several important pieces for one cache level, including
 * the following: an enum class for the Modified, Exclusive, Shared, Invalid (MESI) protocol;
 * a struct for keeping record of each line within the cache; a struct for details regarding a
 * victim due to eviction; a cache class containing several functions to support navigating
 * through the cache.
 * */

#ifndef CACHE_HPP
#define CACHE_HPP

#include <cstdint>
#include <vector>
#include <bit>
#include <optional>
#include "config.hpp"
#include "error.hpp"

enum class Mesi { I, S, E, M };

struct LineRecord {
    uint8_t valid = 0;
    uint64_t tag = 0;
    uint8_t dirty = 0;
    std::vector<uint8_t> data;
    uint64_t inserted = 0;
    uint64_t last_used = 0;
    uint64_t uses = 0;
    Mesi mesi = Mesi::I;
};

struct Victim {
    bool is_dirty = false;
    bool was_valid = false;
    uint64_t victim_addr = 0;
    std::vector<uint8_t> victim_bytes;
};

class Cache {
    private:
        uint32_t block_size;
        uint8_t block_num;
        uint32_t ways;
        uint32_t sets;
        WritePolicy write_policy;
        EvictionPolicy evic_policy;
        std::vector<LineRecord> cache;
        uint64_t counter;
        uint32_t evict_line(uint32_t set);

    public:
        Cache(uint32_t size, uint32_t block_exponent, CacheTypeConfig cache_config, WritePolicy wp, EvictionPolicy ep);
        uint64_t get_block_number(uint64_t paddr);
        uint32_t get_set(uint64_t paddr);
        uint64_t get_tag(uint64_t paddr);
        uint32_t get_offset(uint64_t paddr);
        uint32_t get_block_size();
        WritePolicy get_write_policy();
        Mesi get_state(uint64_t paddr);
        void set_state(uint64_t paddr, Mesi state);
        std::optional<uint32_t> hit(uint64_t paddr);
        std::vector<uint8_t> range_read(uint64_t paddr, uint32_t len);
        void range_write(uint64_t paddr, const std::vector<uint8_t> &bytes);
        Victim install(uint64_t paddr, const std::vector<uint8_t> &bytes);
        void invalidate(uint64_t paddr);
        bool is_dirty(uint64_t paddr);
        bool is_valid(uint64_t paddr);
        void clear_dirty(uint64_t paddr);
        std::optional<std::vector<uint8_t>> get_line(uint64_t paddr);

};

#endif // CACHE_HPP