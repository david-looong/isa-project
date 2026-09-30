#ifndef FETCH_HPP
#define FETCH_HPP

#include "instruction.hpp"
#include "hart.hpp"
#include "config.hpp"
#include <cstdint>

Instruction fetch(Hart& hart);

#endif // FETCH_HPP