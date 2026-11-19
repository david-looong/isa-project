/* Name: David Long
 * Date: November 19, 2026
 * Synopsis: This header file contains a struct that carries information regarding
 * certain system calls as well as functions that support system calls.
 */

#ifndef SYSCALL_HPP
#define SYSCALL_HPP

#include "hart.hpp"
#include "pipeline_object.hpp"
#include <vector>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include <mutex>
#include <tuple>

struct SysCalls {
    bool terminate = false;
    uint32_t exit_code = 0;
    std::ostream* out = &std::cout;
    std::mutex io_mutex;
    std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> sys_record;
};

bool start_pending(SysCalls& sys, uint32_t target);
void apply_hart_changes(std::vector<Hart>& harts, std::vector<HartPipelineObject>& hpos, SysCalls& sys);
void service_syscalls(std::vector<Hart>& harts, std::vector<HartPipelineObject>& hpos, SysCalls& sys);

#endif // SYSCALL_HPP