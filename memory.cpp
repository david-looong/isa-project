#include "memory.hpp"

void memory_rising_edge(Hart &hart, HartPipelineObject &hpo) {
    if (hart.is_parked()) {
        return;
    }

    hpo.mem_wb_next = hpo.ex_mem_current;

//      inst            funct3      memop
//      LB / LBU	    000 / 100	0b001
//      LH / LHU	    001 / 101	0b010
//      LW	            010	        0b011
//      SB	            000	        0b101
//      SH	            001	        0b110
//      SW	            010	        0b111
//      everything else		        0

    uint8_t funct3 = (hpo.mem_wb_next.i.inst & 0x7000) >> 12;

    if (hpo.mem_wb_next.i.memop <= 0b011 && hpo.mem_wb_next.i.memop != 0) {
        if ((static_cast<uint64_t>(hpo.mem_wb_next.i.result) + (1 << (hpo.mem_wb_next.i.memop - 1))) > hart.get_memory_size()) {
            hart.park(ParkReason::Load);
            return;
        }

        hpo.mem_wb_next.i.result = hart.read_memory(hpo.mem_wb_next.i.result, 1 << (hpo.mem_wb_next.i.memop - 1));

        // LBU/LHU (funct3 100/101) are already zero-extended by the read.
        if (funct3 == 0b000)        hpo.mem_wb_next.i.result = sign_extend(hpo.mem_wb_next.i.result, 7);
        else if (funct3 == 0b001)   hpo.mem_wb_next.i.result = sign_extend(hpo.mem_wb_next.i.result, 15);
    }

    return;
}

void memory_falling_edge(Hart &hart, HartPipelineObject &hpo) {
    if (hart.is_parked()) {
        return;
    }

    uint8_t code = hpo.mem_wb_next.i.memop & 0b11;

    if (hpo.mem_wb_next.i.memop >= 0b101) {
        if ((static_cast<uint64_t>(hpo.mem_wb_next.i.result) + (1 << (code - 1))) > hart.get_memory_size()) {
            hart.park(ParkReason::AmoStore);
            return;
        }

        hart.write_memory(hpo.mem_wb_next.i.result, 1 << (code - 1), hpo.mem_wb_next.i.disp);
    }

    hpo.mem_wb_current = hpo.mem_wb_next;

    return;
}