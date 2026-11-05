/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This CPP file contains the contents of the functions previously
 * declared in writeback.hpp.
 * */

#include "writeback.hpp"

// Rising edge for the writeback stage of the instruction pipeline.
void writeback_rising_edge(Hart&, HartPipelineObject &hpo) {
    // Preserves register to be written to and register value to be written.
    hpo.wb_register = hpo.mem_wb_current.i.rd;
    hpo.wb_register_value = hpo.mem_wb_current.i.result;
    return;
}

// Falling edge for the writeback stage of the instruction pipeline.
void writeback_falling_edge(Hart &hart, HartPipelineObject &hpo) {
    // If the register is valid, write the register value to the register.
    if (hpo.wb_register != 0) {
        hart.write_register(hpo.wb_register, hpo.wb_register_value);
    }

    return;
}