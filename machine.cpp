#include "machine.hpp"
#include <vector>

uint32_t Machine::getMemorySize() {
    return mem_size;
}

uint32_t Machine::read(uint32_t addr, uint8_t size) {
    uint32_t mem = 0;

    for (uint8_t i = 0; i < size; i++) {
        mem = mem | (ram[addr + i] << (i * 8));
    }

    return mem;
}

void Machine::write(uint32_t addr, uint8_t size, uint32_t value) {
    for (uint8_t i = 0; i < size; i++) {
        ram[addr + i] = (value >> (i * 8)) & 0xFF;
    }

    return;
}