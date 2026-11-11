/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This CPP file contains the contents of the functions previously
 * declared in memory.hpp.
 * */

#include "memory.hpp"

// Rising edge of the memory stage of the instruction pipeline.
void memory_rising_edge(Hart &hart, HartPipelineObject &hpo) {
    hpo.mem_wb_next = hpo.ex_mem_current;
    hpo.mem_stall = false;
    hpo.csr_write_pending = false;
    MemorySystemResult result;
    uint32_t old_csr_val = 0;
    uint32_t new_csr_val = 0;
    uint32_t csr_source_val = 0;
    uint32_t csr_num = 0;
    uint32_t rs1_field = 0;
    bool csr_will_write = false;

    uint8_t funct3 = (hpo.mem_wb_next.i.inst & 0x7000) >> 12;


    // If no memory access is in progress, proceed with an access. Otherwise, no access.
    if (hpo.mem_wait_current == 0) {

        // In the event we are dealing with a CSR instruction.
         if ((hpo.mem_wb_next.i.inst & 0x7F) == 0x73 && funct3 != 0) {
            csr_num = hpo.mem_wb_next.i.inst >> 20;
            rs1_field = (hpo.mem_wb_next.i.inst >> 15) & 0x1F;

            // If the CSR doesn't actually exist, bubble the instruction and park the HART.
            if (!hart.does_csr_exist(csr_num)) {
                hart.park(ParkReason::IllegalInstruction);
                hpo.mem_wb_next = Latch();
                hpo.mem_wb_next.i.inst = 0x00000013;
                return;
            }
            
            // Determines if the CSR is writable (mhartid and satp, though mhartid is ignored later).
            if (funct3 == 0b001 || funct3 == 0b101 || rs1_field != 0) {
                csr_will_write = true;
            }

            // If it's deemed to write but isn't actually a writable CSR, bubble the instruction
            // and park the HART.
            if (csr_will_write && !hart.is_csr_writable(csr_num)) {
                hart.park(ParkReason::IllegalInstruction);
                hpo.mem_wb_next = Latch();
                hpo.mem_wb_next.i.inst = 0x00000013;
                return;
            }

            // Determine the source value and old CSR value to place in the result field.
            csr_source_val = hpo.mem_wb_next.i.result;
            old_csr_val = hart.get_csr(csr_num);
            hpo.mem_wb_next.i.result = old_csr_val;

            // If deemed writable, the new CSR value is created based off the source value and
            // possibly the old CSR value.
            if (csr_will_write) {
                if (funct3 == 0b001 || funct3 == 0b101) {
                    new_csr_val = csr_source_val;
                } else if (funct3 == 0b010 || funct3 == 0b110) {
                    new_csr_val = old_csr_val | csr_source_val;
                } else if (funct3 == 0b011 || funct3 == 0b111) {
                    new_csr_val = old_csr_val & ~csr_source_val;
                }

                // A pending CSR write is indicated.
                hpo.csr_write_pending = true;
                hpo.csr_write_val = new_csr_val;
                hpo.csr_write_num = csr_num;
            }
         }

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

        // If latency is higher than 1, bubble with NOP in the meantime.
        if (result.latency > 1) {
            hpo.mem_pending = hpo.mem_wb_next;
            hpo.mem_wb_next = Latch();
            hpo.mem_wb_next.i.inst = 0x00000013;
            hpo.mem_stall = true;
            hpo.mem_wait_next = result.latency - 1;
        }
    // The last tick for a memory access. Set the pending memory instruction up for the next tick.
    } else if (hpo.mem_wait_current == 1) {
        hpo.mem_wb_next = hpo.mem_pending;
        hpo.mem_wait_next = 0;
    // Still numerous ticks before another memory access. Bubble with NOP in the meantime.
    } else {
        hpo.mem_wb_next = Latch();
        hpo.mem_wb_next.i.inst = 0x00000013;
        hpo.mem_stall = true;
        hpo.mem_wait_next = hpo.mem_wait_current - 1;
    }

    return;
}

// Falling edge of the memory stage of the instruction pipeline.
void memory_falling_edge(Hart &hart, HartPipelineObject &hpo) {
    // If a CSR write is pending, write the CSR on the falling edge.
    if (hpo.csr_write_pending) {
        hart.set_csr(hpo.csr_write_num, hpo.csr_write_val);
    }

    // Sets up instruction for writeback stage.
    hpo.mem_wb_current = hpo.mem_wb_next;

    // Transfers ticks still left in memory hold as of the start of this tick.
    hpo.mem_wait_current = hpo.mem_wait_next;

    return;
}