/* Name: David Long
 * Date: September 30, 2026
 * Synopsis: This header file contains a declaration for a fetch function that
 * returns an instruction structure.
 */

#ifndef FETCH_HPP
#define FETCH_HPP

#include "instruction.hpp"
#include "hart.hpp"
#include "error.hpp"
#include "pipeline_object.hpp"
#include <cstdint>

Instruction fetch(Hart& hart);
void fetch_rising_edge(Hart& hart, HartPipelineObject& hpo);
void fetch_falling_edge(Hart& hart, HartPipelineObject& hpo);

#endif // FETCH_HPP