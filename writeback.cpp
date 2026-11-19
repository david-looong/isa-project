/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This CPP file contains the contents of the functions previously
 * declared in writeback.hpp.
 * */

#include "writeback.hpp"

// Rising edge for the writeback stage of the instruction pipeline.
void writeback_rising_edge(Hart& hart, HartPipelineObject &hpo) {
    // Resets the syscall flag.
    hpo.request_pending = false;

    // If the instruction is ECALL, gather arguments from the HART's register file
    // and indicate that a syscall request is pending.
    if (hpo.mem_wb_current.i.inst == 0x73)  {
        hpo.a0 = hart.read_register(10);
        hpo.a1 = hart.read_register(11);
        hpo.a2 = hart.read_register(12);
        hpo.a3 = hart.read_register(13);
        hpo.a4 = hart.read_register(14);
        hpo.a5 = hart.read_register(15);
        hpo.a6 = hart.read_register(16);
        hpo.syscall_num = hart.read_register(17);

        hpo.request_pending = true;
    }

    // Preserves register to be written to and register value to be written.
    hpo.wb_register = hpo.mem_wb_current.i.rd;
    hpo.wb_register_value = hpo.mem_wb_current.i.result;
    return;
}

// Falling edge for the writeback stage of the instruction pipeline.
void writeback_falling_edge(Hart &hart, HartPipelineObject &hpo) {
    // If a syscall request is pending, write the return value to a0.
    if (hpo.request_pending)    hart.write_register(10, hpo.syscall_return_val);

    // Write the register value to the register.
    hart.write_register(hpo.wb_register, hpo.wb_register_value);

    return;
}