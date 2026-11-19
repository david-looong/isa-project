/* Name: David Long
 * Date: November 19, 2026
 * Synopsis: This CPP file contains the contents of the previously declared
 * functions in syscall.hpp.
 */

#include "syscall.hpp"

// Determines if target HART already has a pending change in queue.
bool start_pending(SysCalls& sys, uint32_t target) {
    for (size_t i = 0; i < sys.sys_record.size(); i++) {
        if (std::get<0>(sys.sys_record[i]) == target) {
            return true;
        }
    }

    return false;
}

// Applies any changes to the HART. Mainly in the event of start_hart.
void apply_hart_changes(std::vector<Hart>& harts, std::vector<HartPipelineObject>& hpos, SysCalls& sys) {
    for (size_t i = 0; i < sys.sys_record.size(); i++) {
        uint32_t hartid = std::get<0>(sys.sys_record[i]);
        uint32_t a1 = std::get<1>(sys.sys_record[i]);
        uint32_t a2 = std::get<2>(sys.sys_record[i]);
        uint32_t a3 = std::get<3>(sys.sys_record[i]);

        harts[hartid].set_pc(a2);
        harts[hartid].set_csr(0x180, a1);
        harts[hartid].write_register(13, a3);
        hpos[hartid] = HartPipelineObject();
        harts[hartid].unpark();
    }

    // Clear pending HART changes for next ticks.
    sys.sys_record.clear();

    return;
}

// Services any syscalls indicated by the syscall number found in a7.
void service_syscalls(std::vector<Hart>& harts, std::vector<HartPipelineObject>& hpos, SysCalls& sys) {
    for (size_t i = 0; i < harts.size(); i++) {
        // If exit was indicated, do nothing. If no syscall request is pending, go to
        // the next HART.
        if (sys.terminate)  return;
        if (!hpos[i].request_pending)   continue;

        switch(hpos[i].syscall_num) {
            case 0x00: // exit - Grabs the return value and exit code.
                hpos[i].syscall_return_val = hpos[i].a0;
                sys.exit_code = hpos[i].syscall_return_val;
                sys.terminate = true;
                break;
            case 0x10: { // putchar - Writes char value to output stream.
                hpos[i].syscall_return_val = hpos[i].a0;

                // Lock prevents characters from interleaving.
                std::lock_guard<std::mutex> lock(sys.io_mutex);

                sys.out->put(char(hpos[i].syscall_return_val & 0xFF));
                break;
            }
            case 0x11: { // getchar - Reads char value into result.
                pollfd fds = {};
                fds.fd = STDIN_FILENO;
                fds.events = POLLIN;

                // Lock prevents characters from interleaving.
                std::lock_guard<std::mutex> lock(sys.io_mutex);

                // Input redirected from a file is fully deterministic.
                // From terminal, -1 is returned if nothing is typed.
                auto ret = poll(&fds, 1, 0);

                if (ret <= 0) {
                    hpos[i].syscall_return_val = 0xFFFFFFFF;
                } else {
                    unsigned char c;
                    auto got = read(STDIN_FILENO, &c, 1);
                    if (got == 1)   hpos[i].syscall_return_val = uint32_t(c);
                    else            hpos[i].syscall_return_val = 0xFFFFFFFF;
                }
                
                break;
            }
            case 0x20: { // start_hart - Restarts a parked HART.
                uint32_t hartid = hpos[i].a0;

                // If HART doesn't exist, has a pending change, is running, is still draining,
                // or is undergoing a memory operation, write 0 to a0.
                if (hartid >= harts.size() || start_pending(sys, hartid)
                || harts[hartid].get_pr() == ParkReason::Running || hpos[hartid].drain_ticks_left > 0
                || hpos[hartid].mem_wait_current > 0) {
                    hpos[i].syscall_return_val = 0;
                    break;
                }

                // Store information for pending HART change later.
                sys.sys_record.push_back({hartid, hpos[i].a1, hpos[i].a2, hpos[i].a3});
                hpos[i].syscall_return_val = 1;

                break;
            }
            case 0x21: // stop_hart - Parks a HART.
                hpos[i].syscall_return_val = 0;
                harts[i].park(ParkReason::System);
                break;
            case 0x22: { // hart_status - Returns status of a HART.
                uint32_t hartid = hpos[i].a0;

                // Writes -1 to a0 if HART doesn't exist, 0 if a HART change is pending or it's running,
                // or the actual park reason if anything other than running.
                if (hartid >= harts.size()) {
                    hpos[i].syscall_return_val = 0xFFFFFFFF;
                } else if (start_pending(sys, hartid) || harts[hartid].get_pr() == ParkReason::Running) {
                    hpos[i].syscall_return_val = 0;
                } else {
                    hpos[i].syscall_return_val = uint32_t(harts[hartid].get_pr());
                }

                break;
            }
            default: // If no syscall matches accepted ones.
                hpos[i].syscall_return_val = 0xdeadbeef;
                break;
        }
    }

    return;
}