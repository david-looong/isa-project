# RUBRIC.1.md — Part I Self-Assessment

**Name:** David Long

**Date:** 09/18/2026



---

## Category A — Build and Structure (Max: 0.5 pts)

Does the Part I code compile and is the machine core factored into its own
source file(s) as required?

| Step | Criterion | Deduction |
|------|-----------|-----------|
| A1 | Compiles cleanly, machine core in its own file, no external dependencies. | -0.0 |
| A2 | Compiles but emits warnings that indicate real problems (e.g., truncation, unused results on config parsing). | -0.25 |
| A3 | Compiles only after trivial fixes (missing include/import, one typo), OR machine core is not separated into its own file. | -0.5 |
| A4 | Does not compile; requires nontrivial edits to build. | -0.5 (and verify remaining categories by code reading only) |

**Self-assessed step:** A1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
- 
- Build.md
- machine.hpp (Lines 15-24)
- machine.cpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. If you refer to my Build.md file, there are instructions in there for how to compile all my files and run it in order to print out some information. My machine core, which I implemented as its own class, can be found in machine.hpp/.cpp, separate from the rest of the code. Every header used is from the C++ standard library.


---

## Category B — Configuration Parsing and Precedence (Max: 1.0 pts)

Command-line switches, config-file parsing, and the precedence CLI > file > default.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| B1 | All switches parse; config file parses (# comments, whitespace-separated key/value, last-duplicate-wins); precedence is correct; -mem accepts no-suffix/K/M/G. | -0.0 |
| B2 | Minor gap: one or two switches or the -mem suffixes mishandled, but precedence is correct. | -0.5 |
| B3 | Precedence is wrong in one direction (e.g., file overrides CLI, or defaults override the file). | -0.75 |
| B4 | Config file not supported, OR precedence largely non-functional; only some CLI switches work. | -1.0 |
| B5 | Configuration essentially unimplemented (defaults only, switches ignored). | -1.5 |

**Self-assessed step:** B1

**Points claimed:** 1.0

**File(s)/line(s) for verification:**
- config.hpp (Lines 44-106)
- config.cpp (Lines 109-123, 292-395)
- main.cpp (Lines 17-33)

**Justification:** I've completed all criteria in this category for earning full points. All my switches parse correctly after extensive testing, and the config file parsing works as intended as well. My precedence is determined by how I used the functions present in ```config.cpp``` in my ```main.cpp```, and mem accepts no-suffix numeric values as well as suffixes K, M, and G.


---

## Category C — Machine Memory (Max: 0.5 pts)

RAM allocation sized by ```mem```, addressable by byte index, left uninitialized.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| C1 | Allocates exactly ```mem``` bytes; address N indexes byte N; not pre-populated. | -0.0 |
| C2 | Allocated but size is off (rounding, unit error) or wrongly zero-initialized against spec. | -0.25 |
| C3 | Allocation present but not byte-addressable as specified (e.g., word-indexed). | -0.4 |
| C4 | No usable memory allocation. | -0.5 |

**Self-assessed step:** C1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
-
- machine.hpp (Lines 15-24)
- machine.cpp (Lines 14-32)

**Justification:** I've completed all criteria in this category for earning full points. My machine is implemented as a class, and the constructor takes in ```Config.mem``` as an argument to allocate exactly ```mem``` bytes in the ```Machine.ram```. The ```read```/```write``` functions correctly index byte N, which is explicitly stored in little-endian format. 


---

## Category D — Register File and x0 Semantics (Max: 1.5 pts)

The 32x32-bit register file behind a read/write API, with x0 hardwired.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| D1 | 32 registers behind a read/write API; reads of x0 return 0; writes to x0 are discarded; enforced inside the API. | -0.0 |
| D2 | x0 semantics correct, but enforced ad hoc at call sites rather than inside the API (fragile for later parts). | -0.5 |
| D3 | Reads of x0 return 0 but writes to x0 are not discarded (or vice versa). | -0.75 |
| D4 | No read/write API (registers accessed as a raw array), OR x0 not special-cased at all. | -1.0 |
| D5 | Register file missing or wrong width/count. | -1.5 |

**Self-assessed step:** D1

**Points claimed:** 1.5

**File(s)/line(s) for verification:**
-
- register_file.hpp (Lines 14-22)
- register_file.cpp (Lines 9-24)

**Justification:** I've completed all the criteria in this category for earning full points. My registers are implemented as an array of ```uint32_t``` values, and the ```read```/```write``` functions within my ```RegisterFile``` class both account for the special case of register x0.


---

## Category E — PC and CSRs (Max: 1.0 pts)

Program counter and the CSRs.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| E1 | PC present and non-addressable; all CSRs present with correct numbers and read/write behavior (```mhartid``` read-only, ```satp``` hardwired to 0 when ```-mmu off```). | -0.0 |
| E2 | One or two CSRs missing/wrong number, OR satp/mhartid special behavior not honored. | -0.5 |
| E3 | PC missing OR CSRs largely absent. | -0.75 |
| E4 | PC and CSRs essentially unimplemented. | -1.0 |

**Self-assessed step:** E1

**Points claimed:** 1.0

**File(s)/line(s) for verification:**
-
- hart.hpp (Lines 19-58)
- hart.cpp (Lines 21-51)

**Justification:** I've completed all the criteria in this category to earn full points. The PC is a field in my ```Hart``` class, which also includes a ```Csr``` field since I implemented the Csrs as their own class. ```cycle```, ```time```, and ```instret``` each have a 64-bit view since either the upper or lower 32-bits can be returned from the ```read``` function. All Csrs are properly returned in my ```read``` function, and the ```write``` function only allows ```satp``` to be written to if the ```mmu``` is on. Otherwise, it remains 0 as it was initialized in the explicit constructor.


---

## Category F — Sign Extension (Max: 0.5 pts)

A function taking a value and a sign-bit index, extending to 32 bits, with no loops and no conditionals.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| F1 | Correct for all sign-bit positions; contains no loops and no if/else/ternary/switch. | -0.0 |
| F2 | Correct output, but uses a conditional or ternary (violates the branchless restriction). | -0.5 |
| F3 | Correct output, but uses a loop (violates the restriction), OR fails on a subset of sign-bit positions. | -0.75 |
| F4 | Function present but incorrect for the common cases, OR relies on the host to sign-extend. | -1.0 |

**Self-assessed step:** F1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
-
- sign_extend.hpp (Lines 15-30)

**Justification:** I've completed all the criteria in this category for earning full points. My sign_extension function has been extensively tested to ensure it works for all sign-bit positions, and it does not contain any loops or conditional statements. It solely relies on explicit bit operations.


---

## Score Summary

| Category | Max | Earned |
|----------|-----|--------|
| A — Build and Structure | 0.5 | 0.5 |
| B — Configuration & Precedence | 1.0 | 1.0 |
| C — Machine Memory | 0.5 | 0.5 |
| D — Register File & x0 | 1.5 | 1.5 |
| E — PC, SP, and CSRs | 1.0 | 1.0 |
| F — Sign Extension | 0.5 | 0.5 |
| **Total** | **5.0** | **5.0** |
