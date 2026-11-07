#include "memory_system.hpp"

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

// Checks for RAM access faults and either reads from RAM or writes to RAM depending on the access type.
MemorySystemResult MemorySystem::access(uint32_t vaddr, uint8_t size, AccessType type, [[maybe_unused]] uint32_t mhartid, [[maybe_unused]] uint32_t satp, uint32_t store_val, [[maybe_unused]] uint32_t amo_operand, [[maybe_unused]] AtomicOps op) {
    MemorySystemResult result;

    if (static_cast<uint64_t>(vaddr) + size > mem_size) {
        result.pr = ParkReason::Access;
        return result;
    }

    if (type == AccessType::Fetch || type == AccessType::Load) {
        result.data = read(vaddr, size);
    } else if (type == AccessType::Store) {
        write(vaddr, size, store_val);
    }

    result.latency = 1;
    
    return result;
}

// Returns the memory size of the machine.
uint32_t MemorySystem::get_memory_size() {
    return mem_size;
}