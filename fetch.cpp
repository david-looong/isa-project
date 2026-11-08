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

    // Records fetch data. If the latency is greater than 1, fetch needs to stall.
    i.inst = result.data;
    if (result.latency > 1) hpo.fetch_wait = result.latency - 1;
    else                    hpo.fetch_wait = 0;
    hpo.if_id_next.fault = result.pr;

    return i;
}

// Rising edge for the fetch stage of the instruction pipeline.
void fetch_rising_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // If memory has an operation this tick, the fetch stage should be put on hold.
    if (!hpo.fetch_valid && hpo.ex_mem_current.i.memop == 0) {
        // Fetches next instruction and extracts fields for predicting the next PC value.
        hpo.if_id_next.i = fetch(hart, hpo);
        hpo.fetch_valid = true;
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
    // If a fetched instruction has been committed but there is still a hold, decrement the ticks remaining.
    } else if (hpo.fetch_valid && hpo.fetch_wait > 0) {
        hpo.fetch_wait--;
    }

    // Whether or not there is a fetch stall is determined by the fetched instruction being committed or
    // if there are still ticks before the next fetch is valid.
    hpo.fetch_stall = !hpo.fetch_valid || hpo.fetch_wait > 0;

    return;
}

// Falling edge for the fetch stage of the isntruction pipeline.
void fetch_falling_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // If a branch redirect occurs, treat the instruction as a NOP and set the PC to the redirected
    // PC value. If a memory stall or load-use stall occurs, hold. If a fetch stall occurs, bubble
    // with a NOP as well. Otherwise, set the fetched instruction up for the decode stage and move
    // the PC to its predicted value.
    if (hpo.redirect) {
        hpo.if_id_current = Latch();
        hpo.if_id_current.i.inst = 0x00000013;
        hart.set_pc(hpo.redirect_pc);
        hpo.fetch_valid = false;
        hpo.fetch_wait = 0;
        hart.increment_instret();
        hart.increment_csr_cycle();
    } else if (!hpo.mem_stall && hpo.no_stall) {
        if (hpo.fetch_stall) {
            hpo.if_id_current = Latch();
            hpo.if_id_current.i.inst = 0x00000013;
        } else {
            hpo.if_id_current = hpo.if_id_next;
            hart.set_pc(hpo.if_id_next.predicted_next_pc);
            hpo.fetch_valid = false;
            hart.increment_instret();
            hart.increment_csr_cycle();
        }
    }

    return;
}