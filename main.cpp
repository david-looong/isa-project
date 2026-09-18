/* Name: David Long
 * Date: September 18, 2026
 * Synopsis: This CPP file contains the driver code for Part 1 of the project. It parses the command
 * line arguments and configuration file, creates a Machine object, and creates a vector of Hart objects.
 * It then prints out the contents of each Hart's registers, program counter, and Csrs.
 */

#include "machine.hpp"
#include "register_file.hpp"
#include "hart.hpp"
#include "config.hpp"
#include "sign_extend.hpp"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    // Parses the command line arguments and stores them into a map.
    std::map<std::string,std::string> cli_map = parse_cli(argc, argv);
    
    std::map<std::string,std::string> resolved;

    // Parses the configuration file if the -f flag is present and stores the contents into a map.
    if (cli_map.find("f") != cli_map.end()) {
        resolved = parse_config(cli_map["f"]);
    }

    // Overwrites the configuration file values with the command line arguments if they are present.
    for (std::map<std::string,std::string>::iterator it = cli_map.begin(); it != cli_map.end(); it++) {
        resolved[it->first] = it->second;
    }

    // Converts the resolved map into a Config object.
    Config final_config = value_conversion(resolved);

    Machine machine = Machine(final_config.mem);
    std::vector<Hart> harts = {};

    // Creates a Hart object for each hart specified in the configuration and prints out the contents of
    // each Hart's registers, program counter, and Csrs.
    for (uint32_t i = 0; i < final_config.harts; i++) {
        harts.push_back(Hart(machine, i, final_config.mmu));
        std::cout << "***********************" << std::endl;
        std::cout << "HART " << i << " CONTENTS" << std::endl;
        std::cout << "***********************" << std::endl;
        for (uint32_t j = 0; j < static_cast<uint32_t>(32); j++) {
            std::cout << "Register x" << j << " for Hart " << i << ": " << harts[i].read_register(j) << std::endl;
        }
        std::cout << "Program Counter for Hart " << i << ": " << harts[i].get_pc() << std::endl;
        std::cout << "mhartid for Hart " << i << ": " << harts[i].get_csr(0xF14) << std::endl;
        std::cout << "satp for Hart " << i << ": " << harts[i].get_csr(0x180) << std::endl;
        std::cout << "time for Hart " << i << ": " << harts[i].get_csr(0xC01) << std::endl;
        std::cout << "timeh for Hart " << i << ": " << harts[i].get_csr(0xC81) << std::endl;
        std::cout << "cycle for Hart " << i << ": " << harts[i].get_csr(0xC00) << std::endl;
        std::cout << "cycleh for Hart " << i << ": " << harts[i].get_csr(0xC80) << std::endl;
        std::cout << "instret for Hart " << i << ": " << harts[i].get_csr(0xC02) << std::endl;
        std::cout << "instreth for Hart " << i << ": " << harts[i].get_csr(0xC82) << std::endl;
    }
}