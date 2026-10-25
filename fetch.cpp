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
        hart.park(ParkReason::Access);
        return i;
    }

    // Reads the instruction and increments the executable instruction counter.
    i.inst = hart.read_memory(hart.get_pc(), 4);
    hart.increment_instret();

    return i;
}

// Rising edge for the fetch stage of the instruction pipeline.
void fetch_rising_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // Saves fetched instruction and PC value associated with it for decode stage.
    hpo.if_id_next.i = fetch(hart);
    hpo.if_id_next.pc = hart.get_pc();
    return;
}

// Falling edge for the fetch stage of the isntruction pipeline.
void fetch_falling_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // Sets fetched instruction up for decode stage and moves the PC.
    hpo.if_id_current = hpo.if_id_next;
    hart.set_pc(hart.get_pc() + 4);
    return;
}