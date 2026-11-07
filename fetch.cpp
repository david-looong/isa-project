/* Name: David Long
 * Date: September 30, 2026
 * Synopsis: This CPP file contains the fetch function that was previously
 * declared in fetch.hpp.
 */

#include "fetch.hpp"

Instruction fetch(Hart& hart, HartPipelineObject& hpo) {
    Instruction i;

    // Reads the instruction and checks if an Access fault occurs.
    MemorySystemResult result = hart.access(hart.get_pc(), 4, AccessType::Fetch);
    i.inst = result.data;
    hpo.if_id_next.fault = result.pr;

    return i;
}

// Rising edge for the fetch stage of the instruction pipeline.
void fetch_rising_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // Fetches next instruction and extracts fields for predicting the next PC value.
    hpo.if_id_next.i = fetch(hart, hpo);
    InstType type = classify_opcode(hpo.if_id_next.i.inst & 0x7F);
    auto [immediate, rd, funct3, rs1, rs2, funct7] = field_extraction(hpo.if_id_next.i.inst, type);

    // Saves the predicted next PC value based on instruction-type and the branch predictor having
    // taken in its 2-bit history.
    if ((type == InstType::B && hart.get_bp_predict(hart.get_pc()) >= 2) || type == InstType::J) {
        hpo.if_id_next.predicted_next_pc = hart.get_pc() + immediate;
    } else {
        hpo.if_id_next.predicted_next_pc = hart.get_pc() + 4;
    }

    // Extracts the PC.
    hpo.if_id_next.pc = hart.get_pc();
    return;
}

// Falling edge for the fetch stage of the isntruction pipeline.
void fetch_falling_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // If a branch redirect occurs, treat the instruction as a NOP and set the PC to the redirected
    // PC value. Otherwise, set the fetched instruction up for the decode stage and move the PC to its
    // predicted value. In both cases, the number of executable instructions and csr cycles is updated.
    if (hpo.redirect) {
        hpo.if_id_current = Latch();
        hpo.if_id_current.i.inst = 0x00000013;
        hart.set_pc(hpo.redirect_pc);
        hart.increment_instret();
        hart.increment_csr_cycle();
    } else if (hpo.no_stall) {
        hpo.if_id_current = hpo.if_id_next;
        hart.set_pc(hpo.if_id_next.predicted_next_pc);
        hart.increment_instret();
        hart.increment_csr_cycle();
    }

    return;
}