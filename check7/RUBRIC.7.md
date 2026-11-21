# RUBRIC.7.md — Part VII Self-Assessment

**Name:** David Long

**Date:** November 19, 2026

**AI Disclosure:** Claude Opus 5.5 was utilized for discussion/decisions on high-level design decisions and creating test suites to test my code.

**Note on the rubric version:** the TA confirmed (2026-11-18) that where Category E differs from the writeup, the writeup is correct: start_hart takes a0 = HART id, a1 = satp, a2 = entry PC, a3 = argument (placed in the target's a3) and returns 1 on success / 0 on failure; stop_hart takes no parameters and stops the calling HART. Exit codes: a0 for exit, 0 when every HART has stopped, 1 when any HART is parked by a fault.

---

## Category A — Build and Structure (Max: 0.5 pts)

Does the Part VII code compile and is the writeback stage factored into its own source file(s) as required?

| Step | Criterion | Deduction |
|------|-----------|-----------|
| A1 | Compiles cleanly, writeback stage in its own file, no external dependencies. | -0.0 |
| A2 | Compiles but emits warnings that indicate real problems (e.g., truncation, signedness on the committed value, unchecked syscall-number narrowing). | -0.25 |
| A3 | Compiles only after trivial fixes (missing include/import, one typo), OR writeback is not separated into its own file. | -0.5 |
| A4 | Does not compile; requires nontrivial edits to build. | -0.5 (and verify remaining categories by code reading only) |

**Self-assessed step:** A1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
- 
- README.7.md (Entire file)
- writeback.hpp (Entire file)
- writeback.cpp (Entire file)
- syscall.hpp (Entire file)
- syscall.cpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. If you refer to my README.7.md file, there are instructions in there for how to compile all my files and run it. The writeback stage can be found in writeback.hpp/.cpp primarily, and the syscall path can be found in syscall.hpp/.cpp, separate from the rest of the code. Ever header used is from the C++ standard library.


---

## Category B — Register Writeback and x0 Semantics (Max: 2.0 pts)

Commits result to the destination register rd, on the falling edge, with x0 hardwired and non-writing instruction types committing nothing.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| B1 | Writes result to rd in the register file on the falling edge (through the Part I read/write API); writes to x0 are discarded and reads of x0 return 0; B/S types (rd == 0) commit nothing; LOAD commits the memory-stage value that was folded into result in Part VI. | -0.0 |
| B2 | Correct, but a minor issue: writeback issued on the wrong phase (rising edge instead of falling) while the value is still correct on the test set, OR relies on rd == 0 alone without going through the x0-guarded API. | -0.5 |
| B3 | One error: x0 not discarded on write (a spurious write to x0 sticks), OR a non-writing type (B/S) writes a register because rd was not forced to 0. | -1.0 |
| B4 | Register commit not performed through the read/write API (raw array write), OR the committed value is taken from the wrong field so writing instructions store the wrong result. | -1.5 |
| B5 | No usable register writeback; results are not committed to the register file. | -2.0 |

**Self-assessed step:** B1

**Points claimed:** 2.0

**File(s)/line(s) for verification:**
- 
- writeback.cpp (Lines 30-31, 38, 41)
- register_file.cpp (Lines 14-24)
- hart.cpp (Lines 113-121)
- decode.cpp (Lines 78-81, 88-91)

**Justification:** I've completed all criteria in this category for earning full points. The writeback rising edge records rd and result from the current latch (for a load, result holds teh value the memory stage loaded). The falling edge always commits through the register API, and syscall results always goes into x10. Reads of x0 return 0 and writes to x0 are discarded inside the register API, and B/S type instructions set rd = 0, so they don't commit anything.


---

## Category C — Syscall Interface and Dispatch (ABI) (Max: 2.0 pts)

ECALL is dispatched in the writeback stage using the RISC-V calling convention: number in a7, arguments in a0 - a6.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| C1 | ECALL recognized in writeback; the syscall number is read from a7 (x17) and arguments from a0 - a6 (x10 - x16); the call is dispatched to the correct handler; unknown/unsupported numbers are handled gracefully (defined error, no crash); return values placed in a0 as specified. | -0.0 |
| C2 | Dispatch correct, but a minor issue: unknown syscall numbers not handled (silently ignored or fall through) while all supported numbers work, OR return value written somewhere other than a0 in a way the test set tolerates. | -0.5 |
| C3 | One error: number or arguments read from the wrong register(s) (e.g., number from a0), OR ECALL handled outside the writeback stage. | -1.0 |
| C4 | Dispatch keys on something other than the a7 number, OR two or more supported calls route to the wrong handler. | -1.5 |
| C5 | No usable syscall dispatch; ECALL does not reach a handler via the ABI. | -2.0 |

**Self-assessed step:** C1

**Points claimed:** 2.0

**File(s)/line(s) for verification:**
- 
- writeback.cpp (Lines 14-27, 38)
- syscall.hpp (Lines 19-25)
- syscall.cpp (Lines 41-131)
- main.cpp (Line 108)
- decode.cpp (Lines 404-410)
- pipeline_object.hpp (Lines 41-50)

**Justification:** I've completed all criteria in this category for earning full points. ECALL is recognizable in the writeback stage. The syscall number is read from a7, the arguments from a0-a6, and the request is recorded. The syscall dispatch is a switch on the a7 syscall number, containing cases for exit, putchar, getchar, start_hart, stop_hart, and hart_status. Unknown numbers, the default case in the switch, return 0xdeadbeef. The syscall return value is placed in a0 in the writeback falling edge.


---

## Category D — Core Syscalls (exit/putchar/getchar) (Max: 2.0 pts)

Implements the four base system calls with correct behavior and I/O serialized on the shared lock.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| D1 | exit (0) halts using a0 as the exit code; putchar writes the a0 character to output; getchar reads one character from input into a0; putchar/getchar serialize on the shared I/O lock so characters do not interleave. | -0.0 |
| D2 | All four behave correctly, but a minor issue: I/O not serialized on the shared lock (correct single-HART output, races only under concurrency), OR debug emits noise on the graded path. | -0.5 |
| D3 | One core call wrong: putchar/getchar moves the wrong width/register, OR exit ignores the a0 exit code. | -1.0 |
| D4 | exit does not actually terminate the machine, OR two or more core calls are wrong/absent. | -1.5 |
| D5 | Core syscalls not meaningfully implemented. | -2.0 |

**Self-assessed step:** D1

**Points claimed:** 2.0

**File(s)/line(s) for verification:**
- 
- syscall.hpp (Line 23)
- syscall.cpp (Lines 50-85)
- main.cpp (Lines 125, 196-200)

**Justification:** I've completed all criteria in this category for earning full points. The exit switch case sets the exit code from a0 and the terminate flag, which partly determines whether the rising/falling edge loop ends or not. If terminate is set, the output stream is flushed and the program returns a0 as the host exit code. putchar's switch case writes a0's low 8 bits under io_mutex (the shared I/O lock). getchar's switch case has a non-blocking poll() and one-byte read() that gets the input char protected by the io_mutex.


---

## Category E — Multi-HART Bootstrapping (start_hart/stop_hart) (Max: 2.5 pts)

Implements start_hart and stop_hart with phase-aligned wake, the specified ABI, and the required edge-case handling.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| E1 | start_hart wakes a dormant HART: a0=target id, a1=entry PC, a2=argument; target architectural state (PC, argument in its a0) applied on the falling edge and the target begins fetching on the next rising edge; stop_hart re-parks a HART; both return 0 on success or a non-zero error in a0; all edge cases handled (start on running HART, id out of range, stop on dormant HART, self-stop, restart after stop). | -0.0 |
| E2 | Start/stop work, but a minor issue: the target’s state applied on the rising edge (potential race that the test set does not expose), OR one edge case returns success instead of an error without corrupting state. | -0.5 |
| E3 | One class wrong: the wake is not phase-aligned so the started HART races its own decode (intermittent), OR two or more edge cases mishandled (e.g., double-start not rejected, out-of-range id not caught). | -1.25 |
| E4 | start_hart brings a HART up but at the wrong PC/argument (wrong ABI registers), OR stop_hart does not actually re-park the HART. | -1.75 |
| E5 | No usable bootstrapping; non-zero HARTs cannot be started (or cannot be stopped) via the syscalls. | -2.5 |

**Self-assessed step:** E1

**Points claimed:** 2.5

**File(s)/line(s) for verification:**
- 
- syscall.cpp (Lines 10-38 87-122)
- main.cpp (Line 112)
- hart.cpp (Lines 93-96, 131-143)
- fetch.cpp (Lines 27-30, 64-67)
- decode.cpp (Lines 404-410)

**Justification:** I've completed all criteria in this category for earning full points. start_hart doesn't do anything if the target HART is out of range, already running, already started this tick, still draining, or mid memory-hold. In the event it proceeds, it stores a record (hartid, satp, PC, a3) for pending syscall HART changes. It returns 1 on success and 0 on refusal. The syscalls are dealt with in-between the rising/falling edges, and the HART changes are applied after the falling edges so the target fetches from its entry PC on the next rising edge. stop_hart parks the caller with ParkReason::System, and decode keeps it precise by having no load-use stalls if an ECALL is in execute/memory/writeback ticks. hart_status returns -1 if out of range, 0 if running or changes to the HART are pending, and the park reason otherwise. Fetch stage fetches NOP bubbles with frozen PC when HART is parked.


---

## Category F — Termination and Determinism (Max: 1.0 pts)

Machine termination and reproducible ordering of same-tick side effects.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| F1 | exit terminates the whole machine; the machine also halts when every HART is dormant/stopped; shutdown cleanly drains and releases all stage threads (no thread left blocked on the barrier); same-tick side effects (putchar/getchar/start_hart/stop_hart) serialized in ascending HART id order for reproducible output. | -0.0 |
| F2 | Termination correct, but a minor issue: same-tick side effects not serialized in a fixed HART-id order (output ordering non-deterministic across runs) while termination itself is clean. | -0.5 |
| F3 | Termination or shutdown broken: all-dormant does not halt (hang) OR shutdown leaves a thread blocked on the barrier, OR exit stops only the calling HART instead of the machine. | -1.0 |

**Self-assessed step:** F1

**Points claimed:** 1.0

**File(s)/line(s) for verification:**
- 
- main.cpp (Lines 88-93, 102-213)
- syscall.cpp (Lines 42, 46)

**Justification:** I've completed all criteria in this category for earning full points. Exit terminates the entire machine, and the machine halts when every HART is dormant as well. Same-tick side effects are serialized in ascending HART id order, with syscall.cpp:46 stopping service after an exit. Starts are applied after the falling edges.


---

## Score Summary

| Category | Max | Earned |
|----------|-----|--------|
| A — Build and Structure | 0.5 | 0.5 |
| B — Register Writeback and x0 Semantics | 2.0 | 2.0 |
| C — Syscall Interface and Dispatch (ABI) | 2.0 | 2.0 |
| D — Core Syscalls (exit/putchar/getchar) | 2.0 | 2.0 |
| E — Multi-HART Bootstrapping (start_hart/stop_hart) | 2.5 | 2.5 |
| F — Termination and Determinism | 1.0 | 1.0 |
| **Total** | **10.0** | **10.0** |