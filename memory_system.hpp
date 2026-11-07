#ifndef MEMORY_SYSTEM_HPP
#define MEMORY_SYSTEM_HPP

#include <cstdint>
#include <memory>

enum class ParkReason { Running, System, AmoStore, Load, Fetch, IllegalInstruction, Access };
enum class AccessType { Fetch, Load, Store, LR, SC, Amo };
enum class AtomicOps { Swap, Add, And, Or, Xor, Max, MaxU, Min, MinU };

struct MemorySystemResult {
    uint32_t data = 0;
    uint32_t latency = 0;
    ParkReason pr = ParkReason::Running;
};

class MemorySystem {
    private:
        friend class Machine;
        uint32_t mem_size;
        std::unique_ptr<uint8_t[]> ram;
        uint32_t read(uint32_t addr, uint8_t size);
        void write(uint32_t addr, uint8_t size, uint32_t value);
        // L3: one shared cache, plus its mutex (if -cL 3)
        // bus mutex, or the directory (per -cC)
        // per-HART state, indexed by HART id:
        //      L1, L2, TLB, LR/SC reservation
        // the "busy until tick" counter
    public:
        MemorySystem(uint32_t mem_size) : mem_size(mem_size), ram(new uint8_t[mem_size]) {}
        MemorySystemResult access(uint32_t vaddr, uint8_t size, AccessType type, [[maybe_unused]] uint32_t mhartid, [[maybe_unused]] uint32_t satp, uint32_t store_val, [[maybe_unused]] uint32_t amo_operand, [[maybe_unused]] AtomicOps op);
        uint32_t get_memory_size();
};


#endif // MEMORY_SYSTEM_HPP