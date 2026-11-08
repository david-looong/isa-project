/* Name: David Long
 * Date: September 18, 2026
 * Synopsis: This CPP file contains the driver code for the ISA project.
 */

#include "machine.hpp"
#include "register_file.hpp"
#include "hart.hpp"
#include "config.hpp"
#include "sign_extend.hpp"
#include "elf.hpp"
#include "fetch.hpp"
#include "decode.hpp"
#include "execute.hpp"
#include "memory.hpp"
#include "writeback.hpp"
#include <iostream>
#include <vector>
#include <map>

int main(int argc, char* argv[]) {
    // Parses the command line arguments and stores them into a map.
    std::map<std::string,std::string> cli_map = parse_cli(argc, argv);
    
    std::map<std::string,std::string> resolved;

    // Parses the configuration file if the -f flag is present and stores the contents into a map.
    if (cli_map.find("f") != cli_map.end()) {
        resolved = parse_config(cli_map["f"]);
    }

    // Checks if the input ELF file is specified in the command line.
    if (cli_map.find("i") == cli_map.end()) {
        fatal_error("Input ELF file not specified.");
    }

    // Overwrites the configuration file values with the command line arguments if they are present.
    for (std::map<std::string,std::string>::iterator it = cli_map.begin(); it != cli_map.end(); it++) {
        resolved[it->first] = it->second;
    }

    // Converts the resolved map into a Config object.
    Config final_config = value_conversion(resolved);

    Machine machine = Machine(final_config.mem);
    std::vector<Hart> harts = {};
    std::vector<HartPipelineObject> hpos = {};

    // Loads the ELF file into the machine's memory and retrieves the entry point address.
    uint32_t entry = load_elf(resolved["i"], machine);

    // Creates a Hart object for each hart specified in the configuration and sets the PC.
    for (uint32_t i = 0; i < final_config.harts; i++) {
        harts.push_back(Hart(machine, i, final_config.mmu, final_config.bp, final_config.bpD));
        harts[i].set_pc(entry);
        hpos.push_back(HartPipelineObject());
    }

    // Single-threaded clock loop
    uint32_t tick_limit = 10000;
    uint32_t num_ticks = 0;
    uint32_t active_harts;

    while (num_ticks < tick_limit) {
        // If there are no more active HARTs (i.e., the HART is parked and all older instructions have been resolved), stop the loop.
        active_harts = harts.size();
        for (uint32_t i = 0; i < harts.size(); i++) {
            if (harts[i].is_parked() && hpos[i].drain_ticks_left == 0 && hpos[i].mem_wait_current == 0) active_harts--;
        }
        if (active_harts == 0)  break;

        // Do all rising edge phases of each stage in the instruction pipeline per HART.
        for (uint32_t i = 0; i < harts.size(); i++) {
            fetch_rising_edge(harts[i], hpos[i]);
            decode_rising_edge(harts[i], hpos[i], final_config.m, final_config.a);
            execute_rising_edge(harts[i], hpos[i]);
            memory_rising_edge(harts[i], hpos[i]);
            writeback_rising_edge(harts[i], hpos[i]);
        }

        // Do all falling edge phases of each stage in the instruction pipeline per HART.
        for (uint32_t i = 0; i < harts.size(); i++) {
            fetch_falling_edge(harts[i], hpos[i]);
            decode_falling_edge(harts[i], hpos[i]);
            execute_falling_edge(harts[i], hpos[i]);
            memory_falling_edge(harts[i], hpos[i]);
            writeback_falling_edge(harts[i], hpos[i]);
        }

        // Keep count of whether any instruction older than a faulted instruction has finished.
        for (uint32_t i = 0; i < harts.size(); i++) {
            if (harts[i].is_parked() && hpos[i].drain_ticks_left > 0)  hpos[i].drain_ticks_left--;
        }

        num_ticks++;
    }

    // Prints out information on each HART.
    for (uint32_t i = 0; i < harts.size(); i++) {
        std::cout << std::dec << "Hart " << i << ":" << std::endl;
        std::cout << "Is parked? " << harts[i].is_parked() << std::endl;
        std::cout << "Park Reason: " << static_cast<uint32_t>(harts[i].get_pr()) << std::endl;
        std::cout << "Program Counter: " << std::hex << harts[i].get_pc() << std::endl;
        std::cout << "Current Decode/Execute Instruction: " << hpos[i].id_ex_current.i.inst << std::endl;
        std::cout << "Current Decode/Execute PC: " << hpos[i].id_ex_current.pc << std::endl;
    }
}