#ifndef BRANCH_PREDICTOR_HPP
#define BRANCH_PREDICTOR_HPP

#include <cstdint>
#include <vector>

struct BranchEntry {
    uint32_t tag;
    uint8_t state;
    bool valid;
};

class BranchPredictor {
    private:
        uint32_t bp_rows;
        uint32_t default_prediction;
        std::vector<BranchEntry> branch_target_buffer;
    
    public:
        BranchPredictor(uint32_t bp, uint32_t bpD) {
            bp_rows = bp;
            default_prediction = bpD;
            branch_target_buffer.resize(bp_rows);
        }
        uint8_t predict(uint32_t pc);
        void update(uint32_t pc, uint8_t outcome);
};

#endif // BRANCH_PREDICTOR_HPP