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
#include <cstdint>

Instruction fetch(Hart& hart);

#endif // FETCH_HPP