/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This header file yields a Hart and Csr class as well as a ParkReason enum
 * class. Each class has functions that get/set values within the class, with the Hart class
 * possessing its own register file, branch prediction table, reference to the machine, and
 * Csrs.
 */

#ifndef HART_HPP
#define HART_HPP

#include <cstdint>
#include <unordered_map>
#include <string>
#include <chrono>
#include "machine.hpp"
#include "register_file.hpp"
#include "branch_predictor.hpp"
#include "memory_system.hpp"

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
        void increment_instret();
        void increment_cycle();
        bool is_csr_writable(uint32_t num);
        bool does_csr_exist(uint32_t num);
};

class Hart {
    private:
        Csr csrs;
        uint32_t pc;
        RegisterFile rf;
        Machine &m;
        BranchPredictor bp;
        bool parked;
        ParkReason pr;

    public:
        Hart(Machine &machine, uint32_t hart_id, bool mmu, uint32_t bp, uint32_t bpd) : csrs(hart_id, mmu), pc(0), rf(RegisterFile()), m(machine), bp(bp, bpd), parked(false), pr(ParkReason::Running) {}
        uint32_t get_pc();
        void set_pc(uint32_t val);
        uint32_t get_csr(uint32_t num);
        void set_csr(uint32_t num, uint32_t val);
        bool is_csr_writable(uint32_t num);
        bool does_csr_exist(uint32_t num);
        void increment_instret();
        uint32_t read_register(uint32_t index);
        void write_register(uint32_t index, uint32_t value);
        MemorySystemResult access(uint32_t vaddr, uint8_t size, AccessType type, uint32_t store_val = 0, uint32_t amo_operand = 0, AtomicOps op = AtomicOps::Swap);
        void park(ParkReason reason);
        void unpark();
        bool is_parked();
        ParkReason get_pr();
        uint8_t get_bp_predict(uint32_t branch_pc);
        void update_bp(uint32_t branch_pc, uint8_t outcome);
        void increment_csr_cycle();
        void sfence();
};

#endif // HART_HPP