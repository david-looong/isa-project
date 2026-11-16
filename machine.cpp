/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This CPP file contains the contents of the previously declared functions
 * in machine.hpp.
 */

#include "machine.hpp"

// Returns the memory size of the machine.
uint32_t Machine::get_memory_size() {
    return mem_system.get_memory_size();
}

// Checks for RAM access faults and either reads from RAM or writes to RAM depending on the access type.
MemorySystemResult Machine::access(uint32_t vaddr, uint8_t size, AccessType type, uint32_t mhartid, uint32_t satp, uint32_t store_val, uint32_t amo_operand, AtomicOps op) {
    return mem_system.access(vaddr, size, type, mhartid, satp, store_val, amo_operand, op);
}


// Reads a byte, half, or word from RAM.
uint32_t Machine::read(uint32_t addr, uint8_t size) {
    return mem_system.read(addr, size);
}

// Writes a byte, half, or word to RAM.
void Machine::write(uint32_t addr, uint8_t size, uint32_t value) {
    return mem_system.write(addr, size, value);
}

// Flushes the TLB when a SFENCE.VMA occurs.
void Machine::sfence(uint32_t hartid) {
    return mem_system.sfence(hartid);
}