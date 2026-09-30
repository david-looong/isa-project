#include "fetch.hpp"

Instruction fetch(Hart& hart) {
    Instruction i;

    if (hart.get_memory_size() < (static_cast<uint64_t>(hart.get_pc()) + 4)) {
        fatal_error("Out-of-bounds fetch.");
    }

    i.inst = hart.read_memory(hart.get_pc(), 4);
    hart.increment_instret();
    hart.set_pc(hart.get_pc() + 4);

    return i;
}