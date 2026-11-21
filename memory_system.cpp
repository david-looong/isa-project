/* Name: David Long
 * Date: November 15, 2026
 * Synopsis: This CPP file contains the contents of the previously declared functions in
 * memory_system.cpp.
 * */

#include "memory_system.hpp"

// Initializes RAM + cache settings as well as sets up cache levels.
MemorySystem::MemorySystem(uint32_t mem_size, Settings settings) : mem_size(mem_size), ram(new uint8_t[mem_size]), s(settings), granule(granule_size()), cache_mutexes(settings.harts) {
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
    tlbs.resize(s.harts);
    for (size_t i = 0; i < tlbs.size(); i++)    tlbs[i].entries.resize(s.tlb);
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

    std::lock_guard<std::mutex> lock(ram_mutex);

    for (size_t i = 0; i < len; i++) {
        if ((paddr + i) < mem_size) read_data.push_back(ram[paddr + i]);
        else                        read_data.push_back(0);
    }

    return read_data;
}

// Writes a range of bytes to RAM.
void MemorySystem::ram_range_write(uint64_t paddr, const std::vector<uint8_t> &bytes) {
    std::lock_guard<std::mutex> lock(ram_mutex);

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
    std::unique_lock<std::mutex> l3_lock(l3_mutex, std::defer_lock);

    if (level_num == 3) l3_lock.lock();
    
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
                    dir_map_access(vbase).sharers &= ~(1u << hartid);
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
    std::unique_lock<std::mutex> l3_lock(l3_mutex, std::defer_lock);

    if (level_num == 3) l3_lock.lock();
    
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
                    dir_map_access(vbase).sharers &= ~(1u << hartid);
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

// Translates the virtual address into a physical address, and if no faults occur, memory is accessed
// with the physical address.
MemorySystemResult MemorySystem::access(uint32_t vaddr, uint8_t size, AccessType type, [[maybe_unused]] uint32_t mhartid, [[maybe_unused]] uint32_t satp, uint32_t store_val, [[maybe_unused]] uint32_t amo_operand, [[maybe_unused]] AtomicOps op) {
    MemorySystemResult result;
    uint64_t paddr = 0;
    uint64_t latency = 0;

    // If a fault occurs, no memory access can occur.
    result.pr = translate(vaddr, type, mhartid, satp, paddr, latency);
    if (result.pr != ParkReason::Running) {
        return result;
    }
    
    // Account for the latency of the memory operation.
    result = phys_access(paddr, size, type, mhartid, satp, store_val, amo_operand, op);
    result.latency += latency;

    return result;
}

// Translates the virtual address to a physical address for one HART.
ParkReason MemorySystem::translate(uint32_t vaddr, AccessType type, uint32_t hartid, uint32_t satp, uint64_t &paddr, uint64_t &latency) {
    // satp.MODE = 0 means no translation and the TLB is never touched.
    // Also covers -mmu off since Csr never stores satp, so it stays 0.
    if (satp >> 31 == 0) {
        paddr = vaddr;
        return ParkReason::Running;
    }

    PerHartTLB &t = tlbs[hartid];

    // Any change to satp invalidates this HART's TLB and wrrants a TLB flush.
    if (satp != t.last_satp) {
        for (size_t i = 0; i < t.entries.size(); i++)   t.entries[i].valid = false;
        t.last_satp = satp;
    }

    // Determine what kind of page fault occurs if one were to occur.
    ParkReason page_fault_pr;
    if (type == AccessType::Fetch)                                                              page_fault_pr = ParkReason::Fetch;
    else if (type == AccessType::Load || type == AccessType::LR)                                page_fault_pr = ParkReason::Load;
    else if (type == AccessType::Store || type == AccessType::SC || type == AccessType::Amo)    page_fault_pr = ParkReason::AmoStore;

    // Level 1 entries are 4 MiB pages and match on VPN[1] only
    // Level 0 entries match full 20-bit VPN.
    if (s.tlb > 0) {
        for (size_t i = 0; i < t.entries.size(); i++) {
            if (!t.entries[i].valid)    continue;

            bool hit = (t.entries[i].level == 1) ? (vaddr >> 22) == t.entries[i].vpn : (vaddr >> 12) == t.entries[i].vpn;
            
            // Hits add no latency.
            if (hit) {
                t.counter++;
                t.entries[i].last_used = t.counter;
                t.entries[i].uses++;

                // Failing the permission check faults without walking again.
                if (!allowed(t.entries[i].perms, type)) return page_fault_pr;

                if (t.entries[i].level == 1)    paddr = (uint64_t(t.entries[i].ppn) << 22) | (vaddr & 0x3FFFFF);
                else                            paddr = (uint64_t(t.entries[i].ppn) << 12) | (vaddr & 0xFFF);

                return ParkReason::Running;
            }
        }
    }

    uint64_t a = uint64_t(satp & 0x3FFFFF) << 12;
    MemorySystemResult pte_result;

    // Physical load through caches and coherency.
    for (int32_t lvl = 1; lvl >= 0; lvl--) {
        uint64_t vpn = (lvl == 1) ? vaddr >> 22 : (vaddr >> 12) & 0x3FF;
        pte_result = phys_access(a + vpn * 4, 4, AccessType::Load, hartid, satp, 0, 0, AtomicOps::Swap);

        if (pte_result.pr != ParkReason::Running)   return pte_result.pr;
        latency += pte_result.latency;
        uint64_t pte = pte_result.data;

        // Extract permission bits.
        uint8_t V = pte & 0x1;
        uint8_t R = (pte >> 1) & 0x1;
        uint8_t W = (pte >> 2) & 0x1;
        uint8_t X = (pte >> 3) & 0x1;

        if (!V || (!R && W))    return page_fault_pr;
        
        // Marks a leaf. Otherwise, the PTE points to the next table.
        if (R || X) {
            if (!allowed(uint8_t(pte & 0xFF), type))    return page_fault_pr;

            // Only a successful walk fills the TLB.
            if (s.tlb > 0) {
                uint32_t victim = 0;

                // Victim is a free slot first.
                for (size_t i = 0; i < t.entries.size(); i++) {
                    if (!t.entries[i].valid) {
                        victim = i;
                        break;
                    }

                    // TLB eviction policy matches that of cache eviction.
                    switch (s.tlbE) {
                        case EvictionPolicy::fifo:
                            if (t.entries[i].inserted < t.entries[victim].inserted)     victim = i;
                            break;
                        case EvictionPolicy::lru:
                            if (t.entries[i].last_used < t.entries[victim].last_used)   victim = i;
                            break;
                        case EvictionPolicy::lfu:
                            if (t.entries[i].uses < t.entries[victim].uses || (t.entries[i].uses == t.entries[victim].uses && t.entries[i].inserted < t.entries[victim].inserted)) {
                                victim = i;
                            }
                            break;
                        default:
                            break;
                    }
                }

                // Stores the VPN, PPN, leaf level, permission bits, eviction stamps, and validness.
                t.entries[victim].valid = true;

                if (lvl == 1) {
                    t.entries[victim].vpn = vaddr >> 22;
                    t.entries[victim].ppn = pte >> 20;
                } else {
                    t.entries[victim].vpn = vaddr >> 12;
                    t.entries[victim].ppn = pte >> 10;
                }

                t.entries[victim].level = lvl;
                t.entries[victim].perms = pte & 0xFF;
                t.counter++;
                t.entries[victim].inserted = t.counter;
                t.entries[victim].last_used = t.counter;
                t.entries[victim].uses = 1;
            }

            // Level 1 leaf is 4 MiB megapage, so only PPN[1] is kept and offset is 22 bits.
            // Level 0 leaf is 4 KiB with a 12-bit offset
            if (lvl == 1)   paddr = (uint64_t(pte >> 20) << 22) | (vaddr & 0x3FFFFF);
            else            paddr = (uint64_t(pte >> 10) << 12) | (vaddr & 0xFFF);

            return ParkReason::Running;
        }

        // No leaf by level 0; page fault.
        if (lvl == 0)   return page_fault_pr;

        a = uint64_t(pte >> 10) << 12;
    }

    return page_fault_pr;
}

// Flushes all TLB entries in the event of an SFENCE.VMA.
void MemorySystem::sfence(uint32_t hartid) {
    PerHartTLB &t = tlbs[hartid];
    for (size_t i = 0; i < t.entries.size(); i++)   t.entries[i].valid = false;

    return;
}

// Checks for RAM access faults and either reads/writes to/from caches/RAM depending on the access type.
// Bus transactions occur per touched granule before any reads/writes occur, and after the reads/writes,
// the final MESI state is set for each granule.
MemorySystemResult MemorySystem::phys_access(uint64_t paddr, uint8_t size, AccessType type, [[maybe_unused]] uint32_t mhartid, [[maybe_unused]] uint32_t satp, uint32_t store_val, [[maybe_unused]] uint32_t amo_operand, [[maybe_unused]] AtomicOps op) {
    MemorySystemResult result;
    uint64_t deepest = 0;
    uint16_t sharers_mask = 1u << mhartid;
    std::vector<uint8_t> bytes;
    std::vector<std::pair<uint64_t, Mesi>> granules;
    std::unique_lock<std::mutex> bus_lock;
    std::unique_lock<std::mutex> dir_lock_1;
    std::unique_lock<std::mutex> dir_lock_2;
    std::vector<std::unique_lock<std::mutex>> cache_locks;

    // Bounds checking RAM.
    if (paddr + size > mem_size) {
        result.pr = ParkReason::Access;
        return result;
    }

    // LR, SC, and AMO are word-only, so check to make sure that there isn't a misaligned address.
    if ((paddr & 3) != 0 && (type == AccessType::LR || type == AccessType::SC || type == AccessType::Amo)) {
        result.pr = ParkReason::Access;
        return result;
    }

    // If a granule access crosses a boundary, use two granules.
    granules.push_back({granule_base(paddr), Mesi::I});
    if (granules[0].first != granule_base(paddr + size - 1)) {
        granules.push_back({granule_base(paddr + size - 1), Mesi::I});
    }

    // Which lock is taken depends on which coherency protocol is occuring.
    bool coherent = (s.cL >= 1 && s.cC != CacheCoherency::none);

    // Bus lock when coherence protocol is snoop or there is no coherence protocol.
    if (s.cC == CacheCoherency::snoop || !coherent)  bus_lock = std::unique_lock<std::mutex>(bus_mutex);
    
    // If coherence protocol is dir, lock the dir map mutexes based on the number of granules.
    // Obtain the HARTs that will access the cache, and lock the cache locks for those HARTs.
    if (s.cC == CacheCoherency::dir && coherent) {
        for (size_t i = 0; i < granules.size(); i++) {
            if (i == 0) dir_lock_1 = std::unique_lock<std::mutex>(dir_map_access(granules[i].first).dir_lock);
            else        dir_lock_2 = std::unique_lock<std::mutex>(dir_map_access(granules[i].first).dir_lock);
        }

        for (size_t i = 0; i < granules.size(); i++) {
            sharers_mask |= dir_map_access(granules[i].first).sharers.load();
        }

        cache_locks.reserve(s.harts);

        for (uint32_t h = 0; h < s.harts; h++) {
            if ((sharers_mask & (1u << h)) != 0)    cache_locks.emplace_back(cache_mutexes[h]);
        }
    }

    // Determines if a read or write has occurred.
    bool is_read = (type == AccessType::Fetch || type == AccessType::Load || type == AccessType::LR);
    std::unique_lock<std::mutex> lock(reservation_mutex);
    bool is_write = (type == AccessType::Store || (type == AccessType::SC && reservations[mhartid].valid && reservations[mhartid].word_addr == paddr) || type == AccessType::Amo);
    lock.unlock();

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
        bytes = range_read(mhartid, 1, paddr, size, deepest);
        for (size_t i = 0; i < bytes.size(); i++)   result.data = result.data | (bytes[i] << (i * 8));
    } else if (is_write) {
        // If it is an atomic instruction, perform the atomic operation, write the result into the cache,
        // and retain the old word from the cache to return later.
        if (type == AccessType::Amo) {
            uint32_t old_word = 0;
            uint32_t new_word = 0;
            bytes = range_read(mhartid, 1, paddr, size, deepest);

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
            range_write(mhartid, 1, paddr, bytes, deepest);
            result.data = old_word;
        } else {
            for (size_t i = 0; i < size; i++)   bytes.push_back((store_val >> (i * 8)) & 0xFF);
            range_write(mhartid, 1, paddr, bytes, deepest);
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

            if (s.cC == CacheCoherency::dir)    dir_map_access(granules[i].first).sharers |= (1u << mhartid);
        }
    }

    // LR loads and sets the per-HART reservation; SC succeeds only if it writes, otherwise
    // it doesn't write. Either way, the reservation clears.
    lock.lock();
    if (type == AccessType::LR) {
        reservations[mhartid].valid = true;
        reservations[mhartid].word_addr = paddr;
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
            if (hart != mhartid && reservations[hart].valid && byte_ranges_overlap(paddr, reservations[hart].word_addr, size)) {
                    reservations[hart].valid = false;
                    reservations[hart].word_addr = 0;
            }
        }
    }
    lock.unlock();

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
    
    if (s.cC == CacheCoherency::dir)    cur_dir = &dir_map_access(base);
    
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
    std::lock_guard<std::mutex> lock(map_mutex);
    std::unordered_map<uint64_t, DirEntry>::iterator it = dir.find(base);

    if (it != dir.end())    return it->second.sharers.load();
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

// Returns whether [virtual address, virtual address + size] and [word address, word address + 4] overlap or not.
bool MemorySystem::byte_ranges_overlap(uint32_t vaddr, uint32_t word_addr, uint32_t size) {
    return vaddr < word_addr + 4 && word_addr < vaddr + size;
}

// Returns whether a page fault occurs or not based on a combination of permission bits.
bool MemorySystem::allowed(uint8_t perms, AccessType type) {
    uint8_t V = perms & 0x1;
    uint8_t R = (perms >> 1) & 0x1;
    uint8_t W = (perms >> 2) & 0x1;
    uint8_t X = (perms >> 3) & 0x1;

    if (!V || (!R && W))    return false;
    
    if (R || X) {
        if (type == AccessType::Fetch && !X)                                    return false;
        else if ((type == AccessType::Load || type == AccessType::LR) && !R)    return false;
        else if ((type == AccessType::Store || type == AccessType::SC) && !W)   return false;
        else if (type == AccessType::Amo && (!R || !W))                         return false;
    }

    return true;
}

// Locks the dir map and returns a reference to the dir entry associated with the base address / granule.
DirEntry& MemorySystem::dir_map_access(uint64_t base) {
    std::lock_guard<std::mutex> lock(map_mutex);

    return dir[base];
}