/* Name: David Long
 * Date: October 13, 2026
 * Synopsis: This CPP file contains the contents of the function declared in
 * the error.hpp file.
 */

#include "error.hpp"

// Handles fatal errors by printing the error message and exiting the program.
void fatal_error(std::string error) {
    std::cerr << "Fatal error: " << error << std::endl;
    std::exit(1);
}