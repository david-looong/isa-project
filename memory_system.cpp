/* Name: David Long
 * Date: November 15, 2026
 * Synopsis: This CPP file contains the contents of the previously declared functions in
 * memory_system.cpp.
 * */

#include "memory_system.hpp"

// Initializes RAM + cache settings as well as sets up cache levels.
MemorySystem::MemorySystem(uint32_t mem_size, Settings settings) : mem_size(mem_size), ram(new uint8_t[mem_size]), s(settings), granule(granule_size()) {
    if (settings.cL >= 1) {
        for (uint32_t i = 0; i < settings.harts; i++) {
            L1.push_back(Cache(settings.cS1, settings.cB1, settings.c1_type, settings.cW1, settings.cE1));
        }
    }

    if (settings.cL >= 2) {
        for (uint32_t i = 0; i < settings.harts; i++) {
            L2.push_back(Cache(settings.cS2, settings.cB2, settings.c2_type, settings.cW2, settings.cE2));
        }
    }

    if (settings.cL == 3) {
        L3 = Cache(settings.cS3, settings.cB3, settings.c3_type, settings.cW3, settings.cE3);
    }

    reservations.resize(s.harts);
}


// Reads a byte, half, or word from RAM.
uint32_t MemorySystem::read(uint32_t addr, uint8_t size) {
    uint32_t mem = 0;

    for (uint8_t i = 0; i < size; i++) {
        mem = mem | (ram[addr + i] << (i * 8));
    }

    return mem;
}

// Writes a byte, half, or word to RAM.
void MemorySystem::write(uint32_t addr, uint8_t size, uint32_t value) {
    for (uint8_t i = 0; i < size; i++) {
        ram[addr + i] = (value >> (i * 8)) & 0xFF;
    }

    return;
}

// Reads a range of bytes from RAM.
std::vector<uint8_t> MemorySystem::ram_range_read(uint64_t paddr, uint32_t len) {
    std::vector<uint8_t> read_data;

    for (size_t i = 0; i < len; i++) {
        if ((paddr + i) < mem_size) read_data.push_back(ram[paddr + i]);
        else                        read_data.push_back(0);
    }

    return read_data;
}

// Writes a range of bytes to RAM.
void MemorySystem::ram_range_write(uint64_t paddr, const std::vector<uint8_t> &bytes) {
    for (size_t i = 0; i < bytes.size(); i++) {
        if ((paddr + i) < mem_size) ram[paddr + i] = bytes[i];
    }

    return;
}

// Reads a range of bytes at different levels of cache. Recursive in nature.
std::vector<uint8_t> MemorySystem::range_read(uint32_t hartid, uint8_t level_num, uint64_t paddr, uint32_t len, uint64_t &deepest) {
    uint32_t bytes_left = len;
    uint64_t cur_addr = paddr;
    uint32_t block_size;
    uint32_t offset;
    uint32_t room;
    uint32_t piece;
    std::vector<uint8_t> read_data;
    
    // Base case: if the level number is greater than the indicated number of cache levels,
    // we've reached RAM and need to return the bytes read by RAM.
    if (level_num > s.cL) {
        deepest = s.cL + 1;
        return ram_range_read(paddr, len);
    }

    // Tracks the deepest cache level we reach for latency purposes.
    if (level_num > deepest)    deepest = level_num;

    // Walks the range of bytes one line at a time to split line-crossing accesses.
    while (bytes_left > 0) {
        Cache& level = get_cache_level(hartid, level_num);
        block_size = level.get_block_size();
        offset = level.get_offset(cur_addr);
        room = block_size - offset;
        piece = (bytes_left < room) ? bytes_left : room;

        // On a miss, do write-allocation by installing a block from a lower level on the
        // current level and if the victim is dirty, write that block to the lower level as well.
        if (level.hit(cur_addr - offset) == std::nullopt){
            std::vector<uint8_t> install_data;
            install_data.append_range(range_read(hartid, level_num + 1, cur_addr - offset, block_size, deepest));
            Victim victim = level.install(cur_addr, install_data);

            if (victim.is_dirty)    range_write(hartid, level_num + 1, victim.victim_addr, victim.victim_bytes, deepest);
            
            // If directory coherency is used and the victim was valid, check if the HART holds
            // the victim's granule. If not, clear its bit in the directory entry.
            if (s.cC == CacheCoherency::dir && level_num <= 2 && victim.was_valid) {
                uint64_t vbase = granule_base(victim.victim_addr);
                if (!holds_granule(hartid, vbase)) {
                    dir[vbase].sharers &= ~(1u << hartid);
                }
            }
        }

        // Reads indicated bytes on this pass.
        read_data.append_range(level.range_read(cur_addr, piece));

        cur_addr += piece;
        bytes_left -= piece;
    }

    return read_data;
}

// Writes a range of bytes to different levels of cache. Recursive in nature.
void MemorySystem::range_write(uint32_t hartid, uint8_t level_num, uint64_t paddr, const std::vector<uint8_t> &bytes, uint64_t &deepest) {
    uint32_t bytes_left = bytes.size();
    uint64_t cur_addr = paddr;
    uint32_t block_size;
    uint32_t offset;
    uint32_t room;
    uint32_t piece;
    uint32_t cur_line = 0;
    
    // Base case: if the level number is greater than the indicated number of cache levels,
    // we've reached RAM and need to write bytes to RAM.
    if (level_num > s.cL) {
        deepest = s.cL + 1;
        return ram_range_write(paddr, bytes);
    }

    // Tracks the deepest cache level we reach for latency purposes.
    if (level_num > deepest)    deepest = level_num;

    // Walks the range of bytes one line at a time to split line-crossing accesses.
    while (bytes_left > 0) {
        Cache& level = get_cache_level(hartid, level_num);
        block_size = level.get_block_size();
        offset = level.get_offset(cur_addr);
        room = block_size - offset;
        piece = (bytes_left < room) ? bytes_left : room;

        // On a miss, do write-allocation by installing a block from a lower level on the
        // current level and if the victim is dirty, write that block to the lower level as well.
        if (level.hit(cur_addr - offset) == std::nullopt) {
            std::vector<uint8_t> install_data;
            install_data.append_range(range_read(hartid, level_num + 1, cur_addr - offset, block_size, deepest));
            Victim victim = level.install(cur_addr, install_data);

            if (victim.is_dirty)    range_write(hartid, level_num + 1, victim.victim_addr, victim.victim_bytes, deepest);

            // If directory coherency is used and the victim was valid, check if the HART holds
            // the victim's granule. If not, clear its bit in the directory entry.
            if (s.cC == CacheCoherency::dir && level_num <= 2 && victim.was_valid) {
                uint64_t vbase = granule_base(victim.victim_addr);
                if (!holds_granule(hartid, vbase)) {
                    dir[vbase].sharers &= ~(1u << hartid);
                }
            }
        }

        // Writes indicated bytes on this pass. If the write policy is write-through, also write the
        // bytes through all levels of cache.
        std::vector<uint8_t> bytes_piece(bytes.begin() + cur_line, bytes.begin() + cur_line + piece);
        level.range_write(cur_addr, bytes_piece);
        if (level.get_write_policy() == WritePolicy::wt)    range_write(hartid, level_num + 1, cur_addr, bytes_piece, deepest);


        cur_addr += piece;
        cur_line += piece;
        bytes_left -= piece;
    }
    
    return;
}

// Checks for RAM access faults and either reads/writes to/from caches/RAM depending on the access type.
// Bus transactions occur per touched granule before any reads/writes occur, and after the reads/writes,
// the final MESI state is set for each granule.
MemorySystemResult MemorySystem::access(uint32_t vaddr, uint8_t size, AccessType type, [[maybe_unused]] uint32_t mhartid, [[maybe_unused]] uint32_t satp, uint32_t store_val, [[maybe_unused]] uint32_t amo_operand, [[maybe_unused]] AtomicOps op) {
    MemorySystemResult result;
    uint64_t deepest = 0;
    std::vector<uint8_t> bytes;
    std::vector<std::pair<uint64_t, Mesi>> granules;
    std::unique_lock<std::mutex> bus_lock;
    std::unique_lock<std::mutex> dir_lock_1;
    std::unique_lock<std::mutex> dir_lock_2;

    // Bounds checking RAM.
    if (static_cast<uint64_t>(vaddr) + size > mem_size) {
        result.pr = ParkReason::Access;
        return result;
    }

    // LR, SC, and AMO are word-only, so check to make sure that there isn't a misaligned address.
    if ((vaddr & 3) != 0 && (type == AccessType::LR || type == AccessType::SC || type == AccessType::Amo)) {
        result.pr = ParkReason::Access;
        return result;
    }

    // If a granule access crosses a boundary, use two granules.
    granules.push_back({granule_base(vaddr), Mesi::I});
    if (granules[0].first != granule_base(vaddr + size - 1)) {
        granules.push_back({granule_base(vaddr + size - 1), Mesi::I});
    }

    // Which lock is taken depends on which coherency protocol is occuring.
    bool coherent = (s.cL >= 1 && s.cC != CacheCoherency::none);
    if (coherent) {
        if (s.cC == CacheCoherency::snoop) {
            bus_lock = std::unique_lock<std::mutex>(bus_mutex);
        } else {
            for (size_t i = 0; i < granules.size(); i++) {
                if (i == 0) dir_lock_1 = std::unique_lock<std::mutex>(dir[granules[i].first].dir_lock);
                else        dir_lock_2 = std::unique_lock<std::mutex>(dir[granules[i].first].dir_lock);
            }
        }
    }

    // Determines if a read or write has occurred.
    bool is_read = (type == AccessType::Fetch || type == AccessType::Load || type == AccessType::LR);
    bool is_write = (type == AccessType::Store || (type == AccessType::SC && reservations[mhartid].valid && reservations[mhartid].word_addr == vaddr) || type == AccessType::Amo);

    // Only occurs if coherency is activated.
    if ((is_read || is_write) && coherent) {
        // Bus transaction per granule, which determines whether the granule is shared across
        // HARTS or is exclusive only to mhartid.
        for (uint32_t i = 0; i < granules.size(); i++) {
            granules[i].second = my_state(mhartid, granules[i].first);
            if (is_read) {
                if (granules[i].second == Mesi::I) {
                    bool shared = bus_transaction(mhartid, granules[i].first, BusOp::Rd, deepest);
                    if (shared) granules[i].second = Mesi::S;
                    else        granules[i].second = Mesi::E;
                }
            } else if (is_write) {
                if (granules[i].second == Mesi::S || granules[i].second == Mesi::I)   bus_transaction(mhartid, granules[i].first, BusOp::RdX, deepest);
            }
        }
    }

    // Reads if a fetch/load is indicated, writes for stores.
    if (is_read) {
        bytes = range_read(mhartid, 1, vaddr, size, deepest);
        for (size_t i = 0; i < bytes.size(); i++)   result.data = result.data | (bytes[i] << (i * 8));
    } else if (is_write) {
        // If it is an atomic instruction, perform the atomic operation, write the result into the cache,
        // and retain the old word from the cache to return later.
        if (type == AccessType::Amo) {
            uint32_t old_word = 0;
            uint32_t new_word = 0;
            bytes = range_read(mhartid, 1, vaddr, size, deepest);

            for (size_t i = 0; i < bytes.size(); i++)   old_word = old_word | (bytes[i] << (i * 8));

            switch(op) {
                case AtomicOps::Swap:
                    new_word = amo_operand;
                    break;
                case AtomicOps::Add:
                    new_word = old_word + amo_operand;
                    break;
                case AtomicOps::And:
                    new_word = old_word & amo_operand;
                    break;
                case AtomicOps::Or:
                    new_word = old_word | amo_operand;
                    break;
                case AtomicOps::Xor:
                    new_word = old_word ^ amo_operand;
                    break;
                case AtomicOps::Max:
                    new_word = std::max(static_cast<int32_t>(old_word), static_cast<int32_t>(amo_operand));
                    break;
                case AtomicOps::MaxU:
                    new_word = std::max(old_word, amo_operand);
                    break;
                case AtomicOps::Min:
                    new_word = std::min(static_cast<int32_t>(old_word), static_cast<int32_t>(amo_operand));
                    break;
                case AtomicOps::MinU:
                    new_word = std::min(old_word, amo_operand);
                    break;
            }

            bytes.clear();
            for (size_t i = 0; i < size; i++)   bytes.push_back((new_word >> (i * 8)) & 0xFF);
            range_write(mhartid, 1, vaddr, bytes, deepest);
            result.data = old_word;
        } else {
            for (size_t i = 0; i < size; i++)   bytes.push_back((store_val >> (i * 8)) & 0xFF);
            range_write(mhartid, 1, vaddr, bytes, deepest);
        }
    }

    // Sets the final MESI state for the granules. If coherency is directory, the HART directory bit is set.
    if ((is_read || is_write) && coherent) {
        for (uint32_t i = 0; i < granules.size(); i++) {
            if (is_read) {
                set_my_state(mhartid, granules[i].first, granules[i].second);
            } else if (is_write) {
                set_my_state(mhartid, granules[i].first, Mesi::M);
            }

            if (s.cC == CacheCoherency::dir)    dir[granules[i].first].sharers |= (1u << mhartid);
        }
    }

    // LR loads and sets the per-HART reservation; SC succeeds only if it writes, otherwise
    // it doesn't write. Either way, the reservation clears.
    if (type == AccessType::LR) {
        reservations[mhartid].valid = true;
        reservations[mhartid].word_addr = vaddr;
    } else if (type == AccessType::SC) {
        if (is_write) {
            result.data = 0;
        } else {
            result.data = 1;
            result.latency = 1;
        }

        reservations[mhartid].valid = false;
        reservations[mhartid].word_addr = 0;
    }

    // Any store, AMO or successful SC from another HART to that word address clears it.
    if (is_write) {
        for (size_t hart = 0; hart < s.harts; hart++) {
            if (hart != mhartid && reservations[hart].valid && byte_ranges_overlap(vaddr, reservations[hart].word_addr, size)) {
                    reservations[hart].valid = false;
                    reservations[hart].word_addr = 0;
            }
        }
    }

    // Deepest cache level reached indicates the latency of the memory operation.
    if (deepest == s.cL + 1)    result.latency = s.RAM;
    else if (deepest == 1)      result.latency = s.L1;
    else if (deepest == 2)      result.latency = s.L2;
    else if (deepest == 3)      result.latency = s.L3;
    
    return result;
}

// Returns the memory size of the machine.
uint32_t MemorySystem::get_memory_size() {
    return mem_size;
}

// Returns the appropriate cache level based on the indicated level number.
Cache& MemorySystem::get_cache_level(uint32_t hartid, uint32_t level_num) {
    if (level_num == 1)         return L1[hartid];
    else if (level_num == 2)    return L2[hartid];
    else                        return L3.value();
}

// Determines the coherence granularity (i.e., the larger block of the per-HART levels in use).
// Picks the larger block so every smaller line then fits inside exactly one granule.
uint32_t MemorySystem::granule_size() {
    if (s.cL == 1)  return 1u << s.cB1;
    else            return std::max(1u << s.cB1, 1u << s.cB2);
}

// Determines the granule base address.
uint64_t MemorySystem::granule_base(uint64_t paddr) {
    return paddr & ~(granule - 1);
}

// Returns the MESI state of the per-HART cache by searching across L1 and L2.
Mesi MemorySystem::my_state(uint32_t hartid, uint64_t base) {
    uint32_t per_hart_levels = std::min(s.cL, static_cast<uint32_t>(2));

    // Looks through L1 and possibly L2 to pick up first invalid MESI state.
    for (uint32_t level = 1; level <= per_hart_levels; level++) {
        Cache& c = get_cache_level(hartid, level);
        uint32_t block_size = c.get_block_size();
        for (uint64_t addr = base; addr < base + granule; addr += block_size) {
            Mesi state = c.get_state(addr);
            if (state != Mesi::I)   return state;
        }
    }

    return Mesi::I;
}

// Sets the MESI state of the per-HART cache by searching across L1 and L2.
void MemorySystem::set_my_state(uint32_t hartid, uint64_t base, Mesi state) {
    uint32_t per_hart_levels = std::min(s.cL, static_cast<uint32_t>(2));

    for (uint32_t level = 1; level <= per_hart_levels; level++) {
        Cache& c = get_cache_level(hartid, level);
        uint32_t block_size = c.get_block_size();
        for (uint64_t addr = base; addr < base + granule; addr += block_size) {
            c.set_state(addr, state);
        }
    }

    return;
}

// Flushes dirty line records down the cache levels before making every valid line shared or invalidated. 
bool MemorySystem::snoop(uint32_t hartid, uint64_t base, BusOp bus_op, uint64_t& deepest) {
    uint32_t per_hart_levels = std::min(s.cL, static_cast<uint32_t>(2));
    bool valid_line = false;

    for (uint32_t level = 1; level <= per_hart_levels; level++) {
        Cache& c = get_cache_level(hartid, level);
        uint32_t block_size = c.get_block_size();
        for (uint64_t addr = base; addr < base + granule; addr += block_size) {
            if (c.is_valid(addr)) {
                valid_line = true;
                // Flushes a line record down the cache and clears the dirty status.
                if (c.is_dirty(addr)) {
                    range_write(hartid, level + 1, addr, c.get_line(addr).value(), deepest);
                    c.clear_dirty(addr);
                }

                // Sets the state of the line record to shared or invalidates it.
                if (bus_op == BusOp::Rd)        c.set_state(addr, Mesi::S);
                else if (bus_op == BusOp::RdX)  c.invalidate(addr);
            }
        }
    }

    return valid_line;
}

// Single serialization point to mimic threading at this point in time.
// Returns whether any other HART held the granule.
bool MemorySystem::bus_transaction(uint32_t hartid, uint64_t base, BusOp bus_op, uint64_t& deepest) {
    bool any = false;
    DirEntry* cur_dir = nullptr;
    
    if (s.cC == CacheCoherency::dir)    cur_dir = &dir[base];
    
    // Every other HART (or sharers if coherency is dir) is snooped.
    for (uint32_t hart = 0; hart < s.harts; hart++) {
        if (s.cC == CacheCoherency::dir && (cur_dir->sharers & (1u << hart)) == 0) continue;

        if (hart == hartid)  continue;
        any |= snoop(hart, base, bus_op, deepest);
    }

    if (s.cC == CacheCoherency::dir && bus_op == BusOp::RdX)   cur_dir->sharers = uint16_t(1u << hartid);

    return any;
}

// Returns granule's sharer mask (used for tests).
uint16_t MemorySystem::dir_sharers(uint64_t base) {
    std::unordered_map<uint64_t, DirEntry>::iterator it = dir.find(base);

    if (it != dir.end())    return it->second.sharers;
    else                    return 0;
}

// Returns whether a HART has any valid line in the grnaule at L1 or L2.
// Used to decide when to clear a directory bit.
bool MemorySystem::holds_granule(uint32_t hartid, uint64_t base) {
    uint32_t per_hart_levels = std::min(s.cL, static_cast<uint32_t>(2));

    for (size_t level = 1; level <= per_hart_levels; level++) {
        Cache &c = get_cache_level(hartid, level);
        uint32_t block_size = c.get_block_size();
        for (uint64_t addr = base; addr < base + granule; addr += block_size) {
            if (c.is_valid(addr))   return true;
        }
    }

    return false;
}

bool MemorySystem::byte_ranges_overlap(uint32_t vaddr, uint32_t word_addr, uint32_t size) {
    return vaddr < word_addr + 4 && word_addr < vaddr + size;
}