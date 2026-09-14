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
        Hart(Machine &machine, uint32_t hart_id, bool mmu) : csrs(hart_id, mmu), pc(0), sp(machine.getMemorySize() & ~0xF), rf(RegisterFile()), m(machine) {
            rf.write(2, sp);
        }
        uint32_t get_pc();
        uint32_t get_csr(uint32_t num);
        uint32_t read_register(uint32_t index);
};

#endif // HART_HPP