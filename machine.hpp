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

class Machine {
    private:
        uint32_t mem_size;
        std::unique_ptr<uint8_t[]> ram;
    public:
        Machine(uint32_t mem_size) : mem_size(mem_size), ram(new uint8_t[this->mem_size]) {}
        uint32_t getMemorySize();
        uint32_t read(uint32_t addr, uint8_t size);
        void write(uint32_t addr, uint8_t size, uint32_t value);
};

#endif // MACHINE_HPP