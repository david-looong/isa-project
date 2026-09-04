#ifndef SIGN_EXTEND_HPP
#define SIGN_EXTEND_HPP

#include <cstdint>

/*
 * This function performs sign extension on a given unsigned 32-bit integer value based
 * on the specified sign bit index and returns the sign-extended value as a signed 32-bit
 * integer.
 */
inline uint32_t sign_extend(uint32_t value, uint8_t sign_bit_index) {
    // Isolate the sign bit.
    uint32_t sign_bit = (value >> sign_bit_index) & 1;

    // Calculate the number of bits to shift to create the mask for sign extension.
    uint32_t shift_amount = 31 - sign_bit_index;

    // The mask is created by generating a value that has all bits set to 1 from the
    // sign bit index to the most significant bit.
    uint32_t mask = -sign_bit ^ (0xFFFFFFFFu >> shift_amount) * sign_bit;
    
    // Perform the sign extension by XORing the original value with the mask.
    uint32_t extended_value = value ^ mask;

    return extended_value;
}

#endif // SIGN_EXTEND_HPP