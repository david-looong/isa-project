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
    else if (csr_entry == "time")       return static_cast<uint32_t>(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                            std::chrono::system_clock::now().time_since_epoch()).count()));
    else if (csr_entry == "timeh")      return static_cast<uint32_t>((static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                            std::chrono::system_clock::now().time_since_epoch()).count())) >> 32);
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

// Increases the executed instructions counter by one.
void Csr::increment_instret() {
    instret += 1;
    return;
}

// Increments the cycle counter of a Csr.
void Csr::increment_cycle() {
    cycle += 1;
    return;
}

// Determines if the CSR has write permissions.
bool Csr::is_csr_writable(uint32_t num) {
    return (csr_table.find(num) != csr_table.end() && (csr_table.find(num)->second == "mhartid" || csr_table.find(num)->second == "satp"));
}

// Determines if CSR actually exists.
bool Csr::does_csr_exist(uint32_t num) {
    return !(csr_table.find(num) == csr_table.end());
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

// Writes to CSR.
void Hart::set_csr(uint32_t num, uint32_t val) {
    return csrs.write(num, val);
}

// Determines if CSR is writable.
bool Hart::is_csr_writable(uint32_t num) {
    return csrs.is_csr_writable(num);
}

// Determines if CSR actually exists.
bool Hart::does_csr_exist(uint32_t num) {
    return csrs.does_csr_exist(num);
}

// Increases the executed instructions counter by one.
void Hart::increment_instret() {
    return csrs.increment_instret();
}

// Returns the value of a Hart's register.
uint32_t Hart::read_register(uint32_t index) {
    return rf.read(index);
}

// Overwrites a Hart's register with a new value.
void Hart::write_register(uint32_t index, uint32_t value) {
    return rf.write(index, value);
}

// Checks for RAM access faults and either reads from RAM or writes to RAM depending on the access type.
MemorySystemResult Hart::access(uint32_t vaddr, uint8_t size, AccessType type, uint32_t store_val, uint32_t amo_operand, AtomicOps op) {
    uint32_t id = csrs.read(0xF14);
    uint32_t satp = csrs.read(0x180);

    return m.access(vaddr, size, type, id, satp, store_val, amo_operand, op);
}

// Sets the status of a Hart to parked and stores the reason as well.
void Hart::park(ParkReason reason) {
    pr = reason;
    parked = true;
    return;
}

// Sets the status of a Hart to unparked and the status to running again.
void Hart::unpark() {
    pr = ParkReason::Running;
    parked = false;
    return;
}

// Returns if a Hart is parked or not.
bool Hart::is_parked() {
    return parked;
}

// Returns the reason for a parked HART.
ParkReason Hart::get_pr() {
    return pr;
}

// Returns branch prediction.
uint8_t Hart::get_bp_predict(uint32_t branch_pc) {
    return bp.predict(branch_pc);
}

// Updates branch prediction table with new target.
void Hart::update_bp(uint32_t branch_pc, uint8_t outcome) {
    bp.update(branch_pc, outcome);
    return;
}

// Increments the cycle counter of a Csr.
void Hart::increment_csr_cycle() {
    csrs.increment_cycle();
    return;
}

// Flushes the TLB when a SFENCE.VMA occurs.
void Hart::sfence() {
    return m.sfence(csrs.read(0xF14));
}