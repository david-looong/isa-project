#include "hart.hpp"

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

uint32_t Csr::read(uint32_t num) {
    if (csr_table.find(num) == csr_table.end()) return 0;

    std::string csr_entry = csr_table.at(num);

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

void Csr::write(uint32_t num, uint32_t val) {
    if (csr_table.find(num) == csr_table.end() || csr_table.find(num)->second != "satp") {
        return;
    }

    if (mmu_enabled)    satp = val;
    return;
}

uint32_t Hart::get_pc() {
    return pc;
}

uint32_t Hart::read_register(uint32_t index) {
    return rf.read(index);
}