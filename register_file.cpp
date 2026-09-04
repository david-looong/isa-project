#include "register_file.hpp"

// Initializes all 32 registers to 0.
RegisterFile::RegisterFile() {
    registers = {};
}

// Returns value of register except in the case of register x0.
uint32_t RegisterFile::read(uint8_t reg_index) const {
    if (reg_index == 0)     return 0;
    return registers[reg_index];
}

// Writes new value to register except in the case of register x0.
void RegisterFile::write(uint8_t reg_index, uint32_t value) {
    if (reg_index == 0)     return;
    registers[reg_index] = value;
}