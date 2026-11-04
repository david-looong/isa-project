/* Name: David Long
 * Date: September 30, 2026
 * Synopsis: This CPP file contains the contents of the functions present in the
 * BranchPredictor class in branch_predictor.hpp.
 */

#include "branch_predictor.hpp"

uint8_t BranchPredictor::predict(uint32_t pc) {
    // Returns not taken if branch prediction is disabled. Otherwise, if the PC-relative index
    // doesn't have a valid branch entry, or if the PC doesn't match the PC at that index, it
    // returns the default prediction value established by config.
    if (bp_rows == 0) {
        return 0;
    } else if (!branch_target_buffer[pc % bp_rows].valid || branch_target_buffer[pc % bp_rows].tag != pc) {
        return static_cast<uint8_t>(default_prediction);
    }
    
    // Returns the 2-bit state at the PC-relative index.
    return branch_target_buffer[pc % bp_rows].state;
}

void BranchPredictor::update(uint32_t pc, uint8_t outcome) {
    // There's nothing to update if branch prediction is disabled.
    if (bp_rows == 0)   return;
    
    // Updates PC, 2-bit state, and entry validity if no entry is found or if the PCs don't match.
    // Otherwise, the 2-bit state is the only field updated.
    if (!branch_target_buffer[pc % bp_rows].valid || branch_target_buffer[pc % bp_rows].tag != pc) {
        branch_target_buffer[pc % bp_rows].tag = pc;
        branch_target_buffer[pc % bp_rows].state = ((default_prediction << 1) | outcome) & 0b11;
        branch_target_buffer[pc % bp_rows].valid = true;
    } else {
        branch_target_buffer[pc % bp_rows].state = ((branch_target_buffer[pc % bp_rows].state << 1) | (outcome & 1)) & 0b11;
    }

    return;
}