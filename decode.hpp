/* Name: David Long
 * Date: October 21, 2026
 * Synopsis: This header file contains enum classes for instruction types and
 * ALU operations as well as functions that support decoding instructions from
 * RAM.
 */

#ifndef DECODE_HPP
#define DECODE_HPP

#include "error.hpp"
#include "sign_extend.hpp"
#include "hart.hpp"
#include "instruction.hpp"
#include "pipeline_object.hpp"
#include <cstdint>
#include <array>
#include <tuple>
#include <optional>

enum class InstType { R, I, S, B, U, J, Illegal };
enum class AluOp { Add, Sub, Mul, Div, DivU, Rem, RemU, LeftShift, RightShiftA, RightShiftL, Or, Xor, And };

InstType classify_opcode(uint8_t opcode);
std::tuple<uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t> field_extraction(uint32_t inst, InstType type);
std::optional<AluOp> aluop_decode(uint8_t opcode, uint8_t funct3, uint8_t funct7, uint8_t rd, uint8_t rs1, uint8_t srai_bit, uint32_t immediate, bool m, bool a);
Latch decode(Hart& hart, Latch fetched, bool m, bool a);
void decode_rising_edge(Hart& hart, HartPipelineObject& hpo, bool m, bool a);
void decode_falling_edge(Hart& hart, HartPipelineObject& hpo);

#endif // DECODE_HPP