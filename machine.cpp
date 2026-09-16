/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This CPP file contains the contents of the previously declared functions
 * in machine.hpp.
 */

#include "machine.hpp"

// Returns the memory size of the machine.
uint32_t Machine::getMemorySize() {
    return mem_size;
}

// Reads a byte, half, or word from RAM.
uint32_t Machine::read(uint32_t addr, uint8_t size) {
    uint32_t mem = 0;

    for (uint8_t i = 0; i < size; i++) {
        mem = mem | (ram[addr + i] << (i * 8));
    }

    return mem;
}

// Writes a byte, half, or word to RAM.
void Machine::write(uint32_t addr, uint8_t size, uint32_t value) {
    for (uint8_t i = 0; i < size; i++) {
        ram[addr + i] = (value >> (i * 8)) & 0xFF;
    }

    return;
}