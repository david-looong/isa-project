/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This CPP file contains the contents of the functions previously
 * declared in memory.hpp.
 * */

#include "memory.hpp"

// Rising edge of the memory stage of the instruction pipeline.
void memory_rising_edge(Hart &hart, HartPipelineObject &hpo) {
    hpo.mem_wb_next = hpo.ex_mem_current;
    MemorySystemResult result;

    uint8_t funct3 = (hpo.mem_wb_next.i.inst & 0x7000) >> 12;

    // Determines how to access memory depending on if it is a LOAD or STORE.
    if (hpo.mem_wb_next.i.memop <= 0b011 && hpo.mem_wb_next.i.memop != 0) {
        // Loads the value from RAM and checks if a LOAD access fault occurs.
        result = hart.access(hpo.mem_wb_next.i.result, 1 << (hpo.mem_wb_next.i.memop - 1), AccessType::Load);
        
        // If a fault does occur, the HART is parked and the instruction is bubbled with a NOP.
        if (result.pr != ParkReason::Running) {
            hart.park(result.pr);
            hpo.mem_wb_next = Latch();
            hpo.mem_wb_next.i.inst = 0x00000013;
            return;
        } else {
            hpo.mem_wb_next.i.result = result.data;
        }

        // Sign-extend the loaded value if it's a LB or LH instruction.
        // LBU/LHU (funct3 100/101) are already zero-extended by the read.
        if (funct3 == 0b000)        hpo.mem_wb_next.i.result = sign_extend(hpo.mem_wb_next.i.result, 7);
        else if (funct3 == 0b001)   hpo.mem_wb_next.i.result = sign_extend(hpo.mem_wb_next.i.result, 15);
    } else if (hpo.mem_wb_next.i.memop >= 0b101) {
        // Write to RAM the stored value in the disp field and check if a STORE access fault occurs.
        result = hart.access(hpo.mem_wb_next.i.result, 1 << ((hpo.mem_wb_next.i.memop & 0b11) - 1), AccessType::Store, hpo.mem_wb_next.i.disp);

        // If a fault does occur, the HART is parked and the instruction is bubbled with a NOP.
        if (result.pr != ParkReason::Running) {
            hart.park(result.pr);
            hpo.mem_wb_next = Latch();
            hpo.mem_wb_next.i.inst = 0x00000013;
            return;
        }
    }

    return;
}

// Falling edge of the memory stage of the instruction pipeline.
void memory_falling_edge(Hart&, HartPipelineObject &hpo) {
    // Sets up instruction for writeback stage.
    hpo.mem_wb_current = hpo.mem_wb_next;

    return;
}