/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This CPP file contains the contents of the previously declared functions
 * in hart.hpp as well as a lookup table for Csrs.
 */

#include "hart.hpp"

// Lookup table associating numbers with their respective Csr.
std::unordered_map<uint32_t, std::string> csr_table {
    {0xF14, "mhartid"},
    {0x180, "satp"},
    {0xC01, "time"},
    {0xC81, "timeh"},
    {0xC00, "cycle"},
    {0xC80, "cycleh"},
    {0xC02, "instret"},
    {0xC82, "instreth"}
};

// Reads the value of a Csr.
uint32_t Csr::read(uint32_t num) {
    // Returns 0 if the number is invalid.
    if (csr_table.find(num) == csr_table.end()) return 0;

    // Finds the appropriate Csr
    std::string csr_entry = csr_table.at(num);

    // Returns the value of the Csr.
    if (csr_entry == "mhartid")         return mhartid;
    else if (csr_entry == "satp")       return satp;
    else if (csr_entry == "time")       return static_cast<uint32_t>(time);
    else if (csr_entry == "timeh")      return static_cast<uint32_t>(time >> 32);
    else if (csr_entry == "cycle")      return static_cast<uint32_t>(cycle);
    else if (csr_entry == "cycleh")     return static_cast<uint32_t>(cycle >> 32);
    else if (csr_entry == "instret")    return static_cast<uint32_t>(instret);
    else if (csr_entry == "instreth")   return static_cast<uint32_t>(instret >> 32);
    else                                return static_cast<uint32_t>(0);
}

// Overwrites a Csr value with a new value.
void Csr::write(uint32_t num, uint32_t val) {
    // If the number is invalid or is attributed to a read-only Csr, nothing happens.
    if (csr_table.find(num) == csr_table.end() || csr_table.find(num)->second != "satp") {
        return;
    }

    // If the mmu is enabled, then the satp Csr is written to.
    if (mmu_enabled)    satp = val;
    return;
}

// Returns the value of the program counter.
uint32_t Hart::get_pc() {
    return pc;
}

// Sets the program counter to e_entry from ELF header.
void Hart::set_pc(uint32_t val) {
    pc = val;
    return;
}

// Returns the value of a Csr.
uint32_t Hart::get_csr(uint32_t num) {
    return csrs.read(num);
}

// Returns the value of a Hart's register.
uint32_t Hart::read_register(uint32_t index) {
    return rf.read(index);
}

// Overwrites a Hart's register with a new value.
void Hart::write_register(uint32_t index, uint32_t value) {
    return rf.write(index, value);
}

// Returns the value of a Hart's memory at a given address.
uint32_t Hart::read_memory(uint32_t addr, uint8_t size) {
    return m.read(addr, size);
}

// Overwrites a Hart's memory at a given address with a new value.
void Hart::write_memory(uint32_t addr, uint8_t size, uint32_t value) {
    return m.write(addr, size, value);
}