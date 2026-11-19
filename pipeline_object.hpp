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
    Latch mem_pending;
    bool redirect;
    bool no_stall;
    bool branch_resolved_tick;
    bool mem_stall;
    bool fetch_stall;
    bool fetch_valid;
    bool csr_write_pending;
    bool sfence_pending;
    bool request_pending;
    uint32_t syscall_num;
    uint32_t syscall_return_val;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t mem_wait_current;
    uint32_t mem_wait_next;
    uint32_t fetch_wait;
    uint32_t redirect_pc;
    uint8_t wb_register = 0;
    uint32_t wb_register_value = 0;
    uint32_t recorded_branch_pc;
    uint8_t recorded_branch_outcome;
    uint8_t drain_ticks_left;
    uint32_t csr_write_val;
    uint32_t csr_write_num;

    HartPipelineObject() {
        if_id_current.i.inst = 0x00000013;
        if_id_next.i.inst = 0x00000013;
        id_ex_current.i.inst = 0x00000013;
        id_ex_next.i.inst = 0x00000013;
        ex_mem_current.i.inst = 0x00000013;
        ex_mem_next.i.inst = 0x00000013;
        mem_wb_current.i.inst = 0x00000013;
        mem_wb_next.i.inst = 0x00000013;
        mem_pending.i.inst = 0x00000013;
        redirect = false;
        no_stall = true;
        branch_resolved_tick = false;
        mem_stall = false;
        fetch_stall = false;
        fetch_valid = false;
        csr_write_pending = false;
        sfence_pending = false;
        request_pending = false;
        syscall_num = 0;
        syscall_return_val = 0;
        a0 = 0;
        a1 = 0;
        a2 = 0;
        a3 = 0;
        a4 = 0;
        a5 = 0;
        a6 = 0;
        mem_wait_current = 0;
        mem_wait_next = 0;
        fetch_wait = 0;
        redirect_pc = 0;
        recorded_branch_pc = 0;
        recorded_branch_outcome = 0;
        drain_ticks_left = 2;
        csr_write_val = 0;
        csr_write_num = 0;
    }
};

#endif // PIPELINE_OBJECT_HPP