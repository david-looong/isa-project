#include "writeback.hpp"

void writeback_rising_edge(Hart &hart, HartPipelineObject &hpo) {
    hpo.wb_register = hpo.mem_wb_current.i.rd;
    hpo.wb_register_value = hpo.mem_wb_current.i.result;
    return;
}

void writeback_falling_edge(Hart &hart, HartPipelineObject &hpo) {
    if (hpo.wb_register != 0) {
        hart.write_register(hpo.wb_register, hpo.wb_register_value);
    }

    return;
}