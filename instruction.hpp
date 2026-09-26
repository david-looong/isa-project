/* Name: David Long
 * Date: September 25, 2026
 * Synopsis: This header file contains a struct for the instruction structure, containing
 * different fields that are essential to fetching instructions.
 */

#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

#include <cstdint>

struct Instruction {
    uint32_t inst = 0;
    uint32_t left = 0;
    uint32_t right = 0;
    uint32_t result = 0;
    uint32_t disp = 0;
    uint8_t rd = 0;
    uint8_t memop = 0;
    uint8_t aluop = 0;
};

#endif // INSTRUCTION_HPP