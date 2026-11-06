/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This CPP file contains the contents of the functions previously
 * declared in memory.hpp.
 * */

#include "memory.hpp"

// Rising edge of the memory stage of the instruction pipeline.
void memory_rising_edge(Hart &hart, HartPipelineObject &hpo) {
    hpo.mem_wb_next = hpo.ex_mem_current;

    uint8_t funct3 = (hpo.mem_wb_next.i.inst & 0x7000) >> 12;

    // Determines how to access memory depending on if it is a LOAD or STORE.
    if (hpo.mem_wb_next.i.memop <= 0b011 && hpo.mem_wb_next.i.memop != 0) {
        // If there is a LOAD page fault, the HART is parked and the instruction is bubbled with a NOP.
        if ((static_cast<uint64_t>(hpo.mem_wb_next.i.result) + (1 << (hpo.mem_wb_next.i.memop - 1))) > hart.get_memory_size()) {
            hart.park(ParkReason::Load);
            hpo.mem_wb_next = Latch();
            hpo.mem_wb_next.i.inst = 0x00000013;
            return;
        }

        // Loads the value from RAM.
        hpo.mem_wb_next.i.result = hart.read_memory(hpo.mem_wb_next.i.result, 1 << (hpo.mem_wb_next.i.memop - 1));

        // Sign-extend the loaded value if it's a LB or LH instruction.
        // LBU/LHU (funct3 100/101) are already zero-extended by the read.
        if (funct3 == 0b000)        hpo.mem_wb_next.i.result = sign_extend(hpo.mem_wb_next.i.result, 7);
        else if (funct3 == 0b001)   hpo.mem_wb_next.i.result = sign_extend(hpo.mem_wb_next.i.result, 15);
    } else if (hpo.mem_wb_next.i.memop >= 0b101) {
        // If there is a AMO/STORE page fault, the HART is parked and the instruction is bubbled with a NOP.
        if ((static_cast<uint64_t>(hpo.mem_wb_next.i.result) + (1 << ((hpo.mem_wb_next.i.memop & 0b11) - 1))) > hart.get_memory_size()) {
            hart.park(ParkReason::AmoStore);
            hpo.mem_wb_next = Latch();
            hpo.mem_wb_next.i.inst = 0x00000013;
            return;
        }
    }

    return;
}

// Falling edge of the memory stage of the instruction pipeline.
void memory_falling_edge(Hart &hart, HartPipelineObject &hpo) {
    // If instruction is a AMO/STORE, write to RAM the stored value in the disp field.
    if (hpo.mem_wb_next.i.memop >= 0b101) {
        hart.write_memory(hpo.mem_wb_next.i.result, 1 << ((hpo.mem_wb_next.i.memop & 0b11) - 1), hpo.mem_wb_next.i.disp);
    }

    // Sets up instruction for writeback stage.
    hpo.mem_wb_current = hpo.mem_wb_next;

    return;
}