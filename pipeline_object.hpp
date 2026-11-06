/* Name: David Long
 * Date: October 23, 2026
 * Synopsis: This header file contains structs that help transmit instruction information
 * per HART across the rising/falling edge stages of the instruction pipeline.
 */

#ifndef PIPELINE_OBJECT_HPP
#define PIPELINE_OBJECT_HPP

#include "instruction.hpp"
#include "hart.hpp"

struct Latch {
    Instruction i;
    uint32_t pc = 0;
    ParkReason fault = ParkReason::Running;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint32_t predicted_next_pc = 0;
};

struct HartPipelineObject {
    Latch if_id_current;
    Latch if_id_next;
    Latch id_ex_current;
    Latch id_ex_next;
    Latch ex_mem_current;
    Latch ex_mem_next;
    Latch mem_wb_current;
    Latch mem_wb_next;
    bool redirect;
    bool no_stall;
    bool branch_resolved_tick;
    uint32_t redirect_pc;
    uint8_t wb_register = 0;
    uint32_t wb_register_value = 0;
    uint32_t recorded_branch_pc;
    uint8_t recorded_branch_outcome;
    uint8_t drain_ticks_left;

    HartPipelineObject() {
        if_id_current.i.inst = 0x00000013;
        if_id_next.i.inst = 0x00000013;
        id_ex_current.i.inst = 0x00000013;
        id_ex_next.i.inst = 0x00000013;
        ex_mem_current.i.inst = 0x00000013;
        ex_mem_next.i.inst = 0x00000013;
        mem_wb_current.i.inst = 0x00000013;
        mem_wb_next.i.inst = 0x00000013;
        redirect = false;
        no_stall = true;
        branch_resolved_tick = false;
        redirect_pc = 0;
        recorded_branch_pc = 0;
        recorded_branch_outcome = 0;
        drain_ticks_left = 2;
    }
};

#endif // PIPELINE_OBJECT_HPP