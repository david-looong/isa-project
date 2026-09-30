#include "branch_predictor.hpp"

uint8_t BranchPredictor::predict(uint32_t pc) {
    if (bp_rows == 0 || !branch_target_buffer[pc % bp_rows].valid || branch_target_buffer[pc % bp_rows].tag != pc) {
        return static_cast<uint8_t>(default_prediction);
    }
    
    return branch_target_buffer[pc % bp_rows].state;
}

void BranchPredictor::update(uint32_t pc, uint8_t outcome) {
    if (bp_rows == 0)   return;
    
    if (!branch_target_buffer[pc % bp_rows].valid || branch_target_buffer[pc % bp_rows].tag != pc) {
        branch_target_buffer[pc % bp_rows].tag = pc;
        branch_target_buffer[pc % bp_rows].state = ((default_prediction << 1) | outcome) & 0b11;
        branch_target_buffer[pc % bp_rows].valid = true;
    } else {
        branch_target_buffer[pc % bp_rows].state = ((branch_target_buffer[pc % bp_rows].state << 1) | (outcome & 1)) & 0b11;
    }

    return;
}