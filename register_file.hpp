/* Name: David Long
 * Date: September 16, 2026
 * Synopsis: This header file contains a RegisterFile class that yields 32-bit registers.
 * It also includes declarations for a default constructor and read/write functions
 * tht access the registers.
 */

#ifndef REGISTER_FILE_HPP
#define REGISTER_FILE_HPP

#include <cstdint>
#include <array>

class RegisterFile {
    public:
        RegisterFile();
        uint32_t read(uint8_t reg_index) const;
        void write(uint8_t reg_index, uint32_t value);

    private:
        std::array<uint32_t, 32> registers;
};

#endif // REGISTER_FILE_HPP