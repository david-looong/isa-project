/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This header file contains a Machine class that yields the memory size and
 * RAM storage. It declares a constructor, getter function for the memory size, and read
 * and write functions that access RAM.
 */

#ifndef MACHINE_HPP
#define MACHINE_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include "memory_system.hpp"

class Machine {
    private:
        MemorySystem mem_system;
    public:
        Machine(uint32_t mem_size) : mem_system(mem_size) {}
        uint32_t get_memory_size();
        MemorySystemResult access(uint32_t vaddr, uint8_t size, AccessType type, uint32_t mhartid, uint32_t satp, uint32_t store_val, uint32_t amo_operand, AtomicOps op);
        uint32_t read(uint32_t addr, uint8_t size);
        void write(uint32_t addr, uint8_t size, uint32_t value);
};

#endif // MACHINE_HPP