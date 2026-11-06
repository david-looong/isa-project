/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This header file contains functions that support executing instructions
 * after they have been decoded.
 */

#ifndef EXECUTE_HPP
#define EXECUTE_HPP

#include <cstdint>
#include "hart.hpp"
#include "pipeline_object.hpp"
#include "decode.hpp"

struct AluOutput {
    uint32_t result = 0;
    bool N = false;
    bool Z = false;
    bool C = false;
    bool V = false;
};

AluOutput alu(uint32_t left, uint32_t right, uint8_t aluop);
void execute_rising_edge(Hart& hart, HartPipelineObject& hpo);
void execute_falling_edge(Hart& hart, HartPipelineObject& hpo);

#endif // EXECUTE_HPP