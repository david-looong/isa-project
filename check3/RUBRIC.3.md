# RUBRIC.3.md — Part III Self-Assessment

**Name:** David Long

**Date:** 09/30/2026



---

## Category A — Build and Structure (Max: 0.5 pts)

Does the Part III code compile and is the fetch stage factored into its own source file(s) as required?

| Step | Criterion | Deduction |
|------|-----------|-----------|
| A1 | Compiles cleanly, fetch stage in its own file, no external dependencies. | -0.0 |
| A2 | Compiles but emits warnings that indicate real problems (e.g., truncation, signedness, unaligned reads on the instruction word). | -0.25 |
| A3 | Compiles only after trivial fixes (missing include/import, one typo), OR fetch is not separated into its own file. | -0.5 |
| A4 | Does not compile; requires nontrivial edits to build. | -0.5 (and verify remaining categories by code reading only) |

**Self-assessed step:** A1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
-
- main.cpp (Entire file)
- README.3.md (Entire file)
- fetch.hpp (Entire file)
- fetch.cpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. If you refer to my README.3.md file, there are instructions in there for how to compile all my files and run it in order to print out some information. Since no decoding/executing has been implemented yet, I can't show much with the fetching. The fetch stage can be found in fetch.hpp/.cpp, separate from the rest of the code. Every header used is from the C++ standard library.


---

## Category B — Instruction Fetch and Little-Endian Assembly (Max: 3.0 pts)

Reads four bytes starting at the program counter, assembles them into a 32-bit instruction word as little-endian, and advances the PC.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| B1 | Reads exactly 4 bytes at the PC; assembles them little-endian into the 32-bit inst; advances PC by 4 (except where redirected by a branch/jump); starts from e_entry. | -0.0 |
| B2 | Fetch works, but a minor issue: PC advance handled awkwardly (e.g., advanced in the wrong phase) or fetch does not cleanly start from e_entry. | -0.5 |
| B3 | Byte assembly is host-endian rather than explicitly little-endian (works only on little-endian hosts), OR the 4-byte read is not bounds-checked against RAM. | -0.75 |
| B4 | Instruction word assembled at wrong byte order/offsets so the fetched value is systematically wrong, OR PC not advanced. | -1.0 |
| B5 | Instruction not fetched from memory at the PC. | -1.5 |

**Self-assessed step:** B1

**Points claimed:** 3.0

**File(s)/line(s) for verification:**
-
- fetch.hpp (Line 15)
- fetch.cpp (Lines 9-24)
- hart.hpp (Lines 26, 39, 48, 51, 55, 60)
- hart.cpp (Lines 53-67, 74-77, 89-92, 99-102)
- machine.cpp (Lines 14-23)

**Justification:** I've completed all criteria in this category for full points. The fetch is initially bounds-checked and if it passes, the inst field of the instruction structure is assembled in little-endian by reading 4 bytes out of RAM starting at the PC address. Since the bytes are already loaded in little-endian format in the memory buffer, it will correctly produce the bytes in little-endian format when read. The PC is unconditionally advanced by 4 via a setter function since we cannot determine branch/jump instructions currently, and since every PC address is initialized to e_entry, it starts at e_entry.


---

## Category C — Pipeline Instruction Structure (Max: 2.5 pts)

Defines the per-instruction structure handed stage-to-stage, with the correct field layout for later parts; only inst is populated at this stage.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| C1 | Structure defined with all specified fields (inst, left, right, result, disp/strval, rd, memop, aluop) at correct sizes; inst populated by fetch; structure is what gets handed between stages. | -0.0 |
| C2 | Structure present and used, but one or two fields missing/mis-sized, OR fetch populates fields it should not yet touch. | -0.5 |
| C3 | Structure exists but is not the vehicle passed between stages (e.g., fetch writes loose variables instead), so later parts cannot build on it. | -1.0 |
| C4 | No usable pipeline instruction structure. | -2.0 |

**Self-assessed step:** C1

**Points claimed:** 2.5

**File(s)/line(s) for verification:**
-
- instruction.hpp (Lines 12-21)
- fetch.cpp (Line 19)

**Justification:** I've completed all criteria in this category for full points. The instruction structure has been implemented as a struct with all fields declared at correct sizes. inst is the only field that is populated in fetch by reading from RAM at the PC address, and the fetch function returns an instruction object, ensuring that the structure is what gets handed between stages.


---

## Category D — Branch Predictor Table (Max: 3.5 pts)

A branch target buffer keyed on the branch instruction's PC, storing 2-bit state per entry, with the -bdp default for a PC not present in the table.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| D1 | Table keyed on branch PC with -bp rows; 2-bit state per entry; a PC not in the table predicts the -bdp default; -bp 0 disables prediction. | -0.0 |
| D2 | Predictor works, but a minor issue: -bdp default not honored (hardcoded to strongly-not-taken), OR -bp 0 not handled as "disabled." | -0.5 |
| D3 | Table present but keyed/indexed incorrectly (e.g., wrong PC bits, collisions not handled as spec requires), OR only 1 bit of state kept. | -1.5 |
| D4 | Predictor table located or updated incorrectly so predictions are not meaningfully tied to the branch PC. | -2.5 |
| D5 | No branch predictor table. | -3.5 |

**Self-assessed step:** D1

**Points claimed:** 3.5

**File(s)/line(s) for verification:**
-
- branch_predictor.hpp (Lines 16-36)
- branch_predictor.cpp (Lines 12-14, 26-32)

**Justification:** I've completed all criteria in this category for full points. My indexing is based on the PC mod -bp rows, so it is directly associated with the PC. If -bp 0, then the default prediction is returned and nothing gets updated (since there is nothing in the table to udpate). If the PC is not yet in the table, the default prediction is returned as well. There is a 2-bit state per entry included in the branch entry struct created; in the case of a new entry, the 2-bit state is formed with the outcome and the default prediction.


---

## Category E — Shift-Register Implementation (Max: 0.5 pts)

The 2-bit state must be implemented as a shift register, not as a list of if/switch statements (an explicit graded restriction).

| Step | Criterion | Deduction |
|------|-----------|-----------|
| E1 | The 2-bit state is updated as a shift-left register (outcome shifted in); contains no if/switch ladder enumerating the four states. | -0.0 |
| E2 | Predictor produces correct state transitions, but is implemented with an if/switch state machine instead of a shift register (violates the restriction). | -0.5 |
| E3 | Neither a shift register nor a correct state machine (transitions wrong). | -0.5 |

**Self-assessed step:** E1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
-
- branch_predictor.cpp (lines 26-32)

**Justification:** I've completed all criteria in this category for full points. The 2-bit state purely relies on explicit shift AND OR operations instead of conditional statements determining its new value.


---

## Score Summary

| Category | Max | Earned |
|----------|-----|--------|
| A — Build and Structure | 0.5 | 0.5 |
| B — Instruction Fetch & Endianness | 3.0 | 3.0 |
| C — Pipeline Instruction Structure | 2.5 | 2.5 |
| D — Branch Predictor Table | 3.5 | 3.5 |
| E — Shift-Register Implementation | 0.5 | 0.5 |
| **Total** | **5.0** | **5.0** |
