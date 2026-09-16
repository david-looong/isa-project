/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This header file yields a Hart and Csr class. The Csr class contains
 * the eight Csr registers and declares a constructor and read/write functions. The
 * Hart class contains a program counter, stack pointer, a Csr object, a RegisterFile
 * object, and a reference to a Machine object. The class declares a constructor, 
 * getter functions for the program counter and Csrs, and a read register function.
 */

#ifndef HART_HPP
#define HART_HPP

#include <cstdint>
#include <unordered_map>
#include <string>
#include "machine.hpp"
#include "register_file.hpp"

class Csr {
    private:
        uint32_t mhartid;
        uint32_t satp;
        uint64_t cycle;
        uint64_t time;
        uint64_t instret;
        bool mmu_enabled;
    public:
        Csr(uint32_t hart_id, bool mmu) {
            mhartid = hart_id;
            satp = 0;
            cycle = 0;
            time = 0;
            instret = 0;
            mmu_enabled = mmu;
        };
        uint32_t read(uint32_t num);
        void write(uint32_t num, uint32_t val);
};

class Hart {
    private:
        Csr csrs;

        uint32_t pc;
        uint32_t sp;

        RegisterFile rf;

        Machine &m;
    public:
        Hart(Machine &machine, uint32_t hart_id, bool mmu) : csrs(hart_id, mmu), pc(0), sp(), rf(RegisterFile()), m(machine) {}
        uint32_t get_pc();
        uint32_t get_csr(uint32_t num);
        uint32_t read_register(uint32_t index);
};

#endif // HART_HPP