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
#include "syscall.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <barrier>
#include <thread>
#include <chrono>

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

    // Initializes the machine with all cache settings.
    Machine machine = Machine(final_config.mem,
        {   .cL = final_config.cL,
            .harts = final_config.harts,
            .cC = final_config.cC,
            .c1_type = final_config.c1,
            .c2_type = final_config.c2,
            .c3_type = final_config.c3,
            .cS1 = final_config.cS1,
            .cS2 = final_config.cS2,
            .cS3 = final_config.cS3,
            .cB1 = final_config.cB1,
            .cB2 = final_config.cB2,
            .cB3 = final_config.cB3,
            .cW1 = final_config.cW1,
            .cW2 = final_config.cW2,
            .cW3 = final_config.cW3,
            .cE1 = final_config.cE1,
            .cE2 = final_config.cE2,
            .cE3 = final_config.cE3,
            .tlbE = final_config.tlbE,
            .tlb = final_config.tlb
        }
    );

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

    SysCalls sys = {};
    bool stop_flag = false;
    bool phase_flag = false;
    std::ofstream output;
    std::chrono::steady_clock::duration period = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(1.0 / final_config.hz));
    std::chrono::steady_clock::time_point next_tick = std::chrono::steady_clock::now();

    // If output file is indicated, attempt to open the file. If unsuccessful, a fatal error occurs.
    if (resolved.find("o") != resolved.end() && resolved["o"] != "-") {
        output.open(resolved["o"], std::ios::binary);
        if (!output)    fatal_error("Failed to open file for writing.");
        sys.out = &output;
    }

    // On completion of a worker thread's rising or falling edge.
    auto on_completion = [&]() noexcept {
        uint32_t active_harts = harts.size();

        if (!phase_flag) {
            // Services any syscalls picked up from the rising edge of the tick.
            service_syscalls(harts, hpos, sys);
            for (uint32_t i = 0; i < harts.size(); i++) hpos[i].apply_hart_requests(harts[i]);
        } else {
            // Applies any changes to the harts resulting from syscalls.
            apply_hart_changes(harts, hpos, sys);

            // Keep count of whether any instruction older than a faulted instruction has finished.
            for (uint32_t i = 0; i < harts.size(); i++) {
                if (harts[i].is_parked() && hpos[i].drain_ticks_left > 0)  hpos[i].drain_ticks_left--;
            }

            // If there are no more active HARTs (i.e., the HART is parked and all older instructions have been resolved), stop the loop.
            for (uint32_t i = 0; i < harts.size(); i++) {
                if (harts[i].is_parked() && hpos[i].drain_ticks_left == 0 && hpos[i].mem_wait_current == 0) active_harts--;
            }

            // Stop is determined if an exit syscall occurs or there are no more active HARTs.
            stop_flag = (sys.terminate || active_harts == 0);

            // Controls how fast the pipeline moves for each HART.
            if (!stop_flag) {
                next_tick += period;
                if (std::chrono::steady_clock::now() < next_tick)   std::this_thread::sleep_until(next_tick);
                else                                                next_tick = std::chrono::steady_clock::now();
            }
        }

        // Swap the phase from rising to falling or vice versa.
        phase_flag = !phase_flag;
    };

    std::ptrdiff_t num_threads = std::ptrdiff_t(5 * harts.size());
    std::barrier sync_point(num_threads, on_completion);

    // Worker function for each thread. Does the rising/falling edge of indicated stage until an exit has been indicated
    // or if there are no more active harts.
    auto worker = [&](uint32_t hartid, char stage) {
        do {
            switch(stage) {
                case 'f':
                    fetch_rising_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    fetch_falling_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    break;
                case 'd':
                    decode_rising_edge(harts[hartid], hpos[hartid], final_config.m, final_config.a);
                    sync_point.arrive_and_wait();
                    decode_falling_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    break;
                case 'e':
                    execute_rising_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    execute_falling_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    break;
                case 'm':
                    memory_rising_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    memory_falling_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    break;
                case 'w':
                    writeback_rising_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    writeback_falling_edge(harts[hartid], hpos[hartid]);
                    sync_point.arrive_and_wait();
                    break;
            }
        } while (!stop_flag);
    };

    // For each HART, different threads do the rising/falling edges of each stage in the instruciton pipeline.
    std::vector<std::thread> threads;
    for (size_t i = 0; i < harts.size(); i++) {
        threads.emplace_back(worker, i, 'f');
        threads.emplace_back(worker, i, 'd');
        threads.emplace_back(worker, i, 'e');
        threads.emplace_back(worker, i, 'm');
        threads.emplace_back(worker, i, 'w');
    }

    // Join the threads once all work is done.
    for (auto& t : threads) {
        if (t.joinable())   t.join();
    }

    // If an exit syscall was indicated, flush the output stream and return the exit code.
    if (sys.terminate) {
        sys.out->flush();
        return int(sys.exit_code);
    }

    // If any of the HARTs had a fault, flush the output stream and return 1 as the exit code.
    for (uint32_t i = 0; i < harts.size(); i++) {
        if (harts[i].get_pr() != ParkReason::System) {
            sys.out->flush();
            return 1;
        }
    }

    // Flush the output stream and return 0 as the exit code.
    sys.out->flush();
    return 0;
}