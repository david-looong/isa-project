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
};

struct HartPipelineObject {
    Latch if_id_current;
    Latch if_id_next;
    Latch id_ex_current;
    Latch id_ex_next;

    HartPipelineObject() {
        if_id_current.i.inst = 0x00000013;
        if_id_next.i.inst = 0x00000013;
        id_ex_current.i.inst = 0x00000013;
        id_ex_next.i.inst = 0x00000013;
    }
};

#endif // PIPELINE_OBJECT_HPP