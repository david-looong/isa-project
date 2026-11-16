/* Name: David Long
 * Date: November 15, 2026
 * Synopsis: This CPP file includes the contents of the previously declared functions in
 * cache.hpp.
 * */

#include "cache.hpp"

// Sets up information for the specific cache level.
Cache::Cache(uint32_t size, uint32_t block_exponent, CacheTypeConfig cache_config, WritePolicy wp, EvictionPolicy ep) {
    // Bounds checking for the block exponent.
    if (block_exponent >= 32)   fatal_error("Invalid block exponent.");

    block_size = 1u << block_exponent;
    block_num = block_exponent;

    // Initializes ways depending on the associativity of the cache level.
    if (cache_config.type == CacheType::dm) {
        ways = 1;
    } else if (cache_config.type == CacheType::sa) {
        if (cache_config.ways_exponent >= 32)   fatal_error("Invalid ways exponent.");
        ways = 1u << cache_config.ways_exponent;
    }
    else if (cache_config.type == CacheType::fa) {
        ways = size / block_size;
    }

    // Bounds checking for the cache size.
    if (size < block_size || size % (static_cast<uint64_t>(block_size) * ways) != 0 || size / (static_cast<uint64_t>(block_size) * ways) == 0) {
        fatal_error("Invalid size.");
    }

    sets = size / (block_size * ways);
    write_policy = wp;
    evic_policy = ep;
    counter = 0;

    cache.resize(sets * ways);
    for (uint32_t i = 0; i < cache.size(); i++) cache[i].data.resize(block_size);
}

// Returns the block number of a physical address.
uint64_t Cache::get_block_number(uint64_t paddr) {
    return paddr >> block_num;
}

// Returns the set number of a physical address.
uint32_t Cache::get_set(uint64_t paddr) {
    return get_block_number(paddr) % sets;
}

// Returns the tag of a physical address.
uint64_t Cache::get_tag(uint64_t paddr) {
    return get_block_number(paddr) / sets;
}

// Returns the offset of a physical address.
uint32_t Cache::get_offset(uint64_t paddr) {
    return paddr & (block_size - 1);
}

// Returns the block size of the cache level.
uint32_t Cache::get_block_size() {
    return block_size;
}

// Returns the write policy of the cache level.
WritePolicy Cache::get_write_policy() {
    return write_policy;
}

// Returns the MESI state of a line record.
Mesi Cache::get_state(uint64_t paddr) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt) return cache[line_hit.value()].mesi;

    return Mesi::I;
}

// Sets the MESI state of a line record.
void Cache::set_state(uint64_t paddr, Mesi state) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt) cache[line_hit.value()].mesi = state;

    return;
}

// Determines if a hit or miss occurs in the cache.
std::optional<uint32_t> Cache::hit(uint64_t paddr) {
    uint32_t set = get_set(paddr);
    
    // Return the line index if a hit occurs.
    for (uint32_t i = set * ways; i < set * ways + ways; i++) {
        if (cache[i].valid && get_tag(paddr) == cache[i].tag)   return i;
    }

    return std::nullopt;
}

// Reads a range of bytes from a line record within the cache.
std::vector<uint8_t> Cache::range_read(uint64_t paddr, uint32_t len) {
    uint32_t offset = get_offset(paddr);
    std::vector<uint8_t> read_data;
    std::optional<uint32_t> line_hit = hit(paddr);
    uint32_t line_rec = line_hit.value();

    for (uint32_t i = 0; i < len; i++) {
        read_data.push_back(cache[line_rec].data[offset + i]);
    }

    counter += 1;
    cache[line_rec].last_used = counter;
    cache[line_rec].uses += 1;

    return read_data;
}

// Writes a range of bytes to a line record within the cache.
void Cache::range_write(uint64_t paddr, const std::vector<uint8_t> &bytes) {
    uint32_t offset = get_offset(paddr);
    std::optional<uint32_t> line_hit = hit(paddr);
    uint32_t line_rec = line_hit.value();

    for (size_t i = 0; i < bytes.size(); i++) {
        cache[line_rec].data[offset + i] = bytes[i];
    }

    counter += 1;
    cache[line_rec].last_used = counter;
    cache[line_rec].uses += 1;
    if (write_policy == WritePolicy::wb)    cache[line_rec].dirty = 1;

    return;
}

// Determines which line record gets evicted based on eviction policy.
uint32_t Cache::evict_line(uint32_t set) {
    uint32_t victim = set * ways;

    // If a line record isn't even valid yet, return that index. Otherwise, refer to
    // eviction policy rules to determine the evicted line record.
    for (uint32_t i = set * ways; i < set * ways + ways; i++) {
        if (!cache[i].valid)    return i;

        switch (evic_policy) {
            case EvictionPolicy::fifo:
                if (cache[i].inserted < cache[victim].inserted)     victim = i;
                break;
            case EvictionPolicy::lru:
                if (cache[i].last_used < cache[victim].last_used)   victim = i;
                break;
            case EvictionPolicy::lfu:
                if (cache[i].uses < cache[victim].uses || (cache[i].uses == cache[victim].uses && cache[i].inserted < cache[victim].inserted)) {
                    victim = i;
                }
                break;
            default:
                break;
        }
    }

    return victim;
}

// Installs a line record after potentially evicting an old one.
Victim Cache::install(uint64_t paddr, const std::vector<uint8_t> &bytes) {
    Victim victim;
    uint32_t victim_line = evict_line(get_set(paddr));

    // If there is a line record and it's marked dirty, save it for being moved elsewhere.
    if (cache[victim_line].valid) {
        victim.victim_addr = (cache[victim_line].tag * sets + get_set(paddr)) << block_num;
        victim.victim_bytes = cache[victim_line].data;
        victim.was_valid = true;
        if (cache[victim_line].dirty)   victim.is_dirty = true;
    }

    // Install the new line record where the victim was previously.
    cache[victim_line].valid = 1;
    cache[victim_line].tag = get_tag(paddr);
    cache[victim_line].data = bytes;
    cache[victim_line].dirty = 0;
    counter++;
    cache[victim_line].inserted = counter;
    cache[victim_line].last_used = counter;
    cache[victim_line].uses = 1;
    cache[victim_line].mesi = Mesi::I;

    return victim;
}

// Invalidates a line record in the cache.
void Cache::invalidate(uint64_t paddr) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt) {
        cache[line_hit.value()].mesi = Mesi::I;
        cache[line_hit.value()].valid = 0;
    }

    return;
}

// Returns whether or not a line record is marked dirty in the cache.
bool Cache::is_dirty(uint64_t paddr) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt)   return cache[line_hit.value()].dirty;

    return false;
}

// Returns whether or not a line record is marked valid in the cache.
bool Cache::is_valid(uint64_t paddr) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt)   return cache[line_hit.value()].valid;

    return false;
}

// Clears the dirty status of a line record.
void Cache::clear_dirty(uint64_t paddr) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt)   cache[line_hit.value()].dirty = 0;

    return;
}

// Returns the bytes of a line record (if valid).
std::optional<std::vector<uint8_t>> Cache::get_line(uint64_t paddr) {
    std::optional<uint32_t> line_hit = hit(paddr);
    if (line_hit != std::nullopt)   return cache[line_hit.value()].data;

    return std::nullopt;
}