/* Name: David Long
 * Date: September 30, 2026
 * Synopsis: This CPP file contains the fetch function that was previously
 * declared in fetch.hpp.
 */

#include "fetch.hpp"

Instruction fetch(Hart& hart) {
    Instruction i;

    // Bounds-checking to see if fetch would go past RAM.
    if (hart.get_memory_size() < (static_cast<uint64_t>(hart.get_pc()) + 4)) {
        fatal_error("Out-of-bounds fetch.");
    }

    // Reads the instruction, increments the executable instruction counter, and
    // advances the program counter by 4 bytes.
    i.inst = hart.read_memory(hart.get_pc(), 4);
    hart.increment_instret();
    hart.set_pc(hart.get_pc() + 4);

    return i;
}