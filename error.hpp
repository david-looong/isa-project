/* Name: David Long
 * Date: October 13, 2026
 * Synopsis: This header file contains the declaratoin for a fatal error function
 * that is utilized throughout multiple files to print out specific errors before
 * exiting the program.
 */

#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <cstdlib>
#include <iostream>

// Fatal error function
void fatal_error(std::string error);

#endif // ERROR_HPP