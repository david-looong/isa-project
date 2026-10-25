# RUBRIC.4.md — Part IV Self-Assessment

**Name:** David Long

**Date:** October 25, 2026



---

## Category A — Build and Structure (Max: 0.5 pts)

Does the Part IV code compile and is the decode stage factored into its own source file(s) as required?

| Step | Criterion | Deduction |
|------|-----------|-----------|
| A1 | Compiles cleanly, decode stage in its own file, no external dependencies. | -0.0 |
| A2 | Compiles but emits warnings that indicate real problems (e.g., truncation, signedness on immediate assembly, unaligned field reads). | -0.25 |
| A3 | Compiles only after trivial fixes (missing include/import, one typo), OR decode is not separated into its own file. | -0.5 |
| A4 | Does not compile; requires nontrivial edits to build. | -0.5 (and verify remaining categories by code reading only) |

**Self-assessed step:** A1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
-
- README.4.md (Entire file)
- main.cpp (Entire file)
- decode.hpp (Entire file)
- decode.cpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. If you refer to my README.4.md file, there are instructions in there for how to compile all my files and run it in order to print out some information. The decode stage can be found in decode.hpp/.cpp, separate from the rest of the code. Every header used is from the C++ standard library.


---

## Category B — Opcode Decode and Type Classification (Max: 1.5 pts)

Reads the 7-bit opcode and classifies each instruction into exactly one of the six types (R, I, S, B, U, J).

| Step | Criterion | Deduction |
|------|-----------|-----------|
| B1 | Extracts the 7-bit opcode (inst[6:0]); classifies every supported instruction into the correct single type; no opcode resolves to more than one type; unknown opcodes flagged as illegal. | -0.0 |
| B2 | Classification correct, but a minor issue: illegal/unknown opcodes not detected (silently mis-typed), OR one category label wrong while the field layout still matches. | -0.5 |
| B3 | One opcode systematically mapped to the wrong type (e.g., JALR treated as J), so that instruction class decodes incorrectly. | -0.75 |
| B4 | Type classification driven by something other than the opcode (e.g., guessing from funct3), OR two or more opcodes mis-typed. | -1.0 |
| B5 | No usable opcode-based type classification. | -1.5 |

**Self-assessed step:** B1

**Points claimed:** 1.5

**File(s)/line(s) for verification:**
-
- decode.hpp (Lines 21, 24)
- decode.cpp (Lines 9-35, 275-280)
- hart.hpp (Line 65)
- hart.cpp (Lines 104-109)

**Justification:** I've completed all criteria in this category for full points. An opcode lookup table of 128 entries (2^8 for 8 bits in the opcode) is populated with enum values representing each supported instruction type or an IllegalInstruction type. The opcode is passed in and returns the proper instruction type. If the instruction is marked illegal, the HART is parked and decode stops.


---

## Category C — Field Extraction and Immediate Reconstruction (Max: 2.5 pts)

Extracts rs1, rs2, rd, funct3, funct7, and reassembles each type's immediate from its scattered bit fields, sign-extended to 32 bits via the Part I sign_extend.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| C1 | All register/funct fields extracted at correct positions; I/S/B/U/J immediates reassembled correctly (including the scrambled imm[12\|10:5] / imm[4:1\|11] for B and imm[20\|10:1\|11\|19:12] for J); each immediate sign-extended to 32 bits with sign_extend. | -0.0 |
| C2 | Immediates correct, but a minor issue: shamt for SLLI/SRLI/SRAI not masked to 5 bits, OR one register/funct field read at a slightly wrong position that does not break the test set. | -0.5 |
| C3 | One immediate type reconstructed incorrectly (most commonly B or J bit-scrambling), OR U-type immediate not placed in the upper 20 bits. | -1.0 |
| C4 | Immediate sign extension omitted or done by the host type (works only for positive immediates), OR two or more immediate types reconstructed incorrectly. | -1.75 |
| C5 | Immediates and/or register fields not meaningfully extracted. | -2.5 |

**Self-assessed step:** C1

**Points claimed:** 2.5

**File(s)/line(s) for verification:**
-
- decode.hpp (Lines 25, 27)
- decode.cpp (Lines 37-128, 283, 319-324)
- sign_extend.hpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. All register, funct, and immediate fields are bit masked and shift appropriately based on the instruction type. When returning the extracted fields, the immediate is passed through the sign_extend function created in Part I. The shamt values are masked based on the assigned ALU operation later on when the instruction structure fields are being filled in later on (Category D contents).


---

## Category D — Operand Selection and Register-File Reads (Max: 2.5 pts)

Populates the left / right ALU inputs and strval per instruction type, reading the register file on the rising edge.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| D1 | Correct operands per type: R uses rs1/rs2; I uses rs1 + immediate; S uses rs1 (base) + immediate with the store value from rs2 in strval; B uses rs1/rs2; register file read on the rising edge; rd captured for writing types and set to 0 for B/S. | -0.0 |
| D2 | Operands correct, but read timing is wrong (register file read on the falling edge or outside the two-phase discipline), OR rd not zeroed for B/S types. | -0.5 |
| D3 | One operand-selection error: store value taken from the wrong register, OR I-type right operand taken from a register instead of the immediate, OR base/offset swapped for S. | -1.0 |
| D4 | Operands not routed through the left / right / strval structure fields (e.g., decoded into loose variables later parts cannot consume), OR register file not read via the read/write API. | -1.75 |
| D5 | Operands not selected/populated in a usable way. | -2.5 |

**Self-assessed step:** D1

**Points claimed:** 2.5

**File(s)/line(s) for verification:**
-
- decode.hpp (Lines 27-28)
- decode.cpp (Lines 284, 293, 295-363, 368-378)
- main.cpp (Lines 71-75)

**Justification:** I've completed all criteria in this category for earning full points. All operands are correctly assigned based on the instruction type, and it occurs on the rising edge of the decode stage. A lot of zeroed out fields occur in the field_extraction function referenced for the Category C justificaiton.


---

## Category E — ALU Operation Decode (Max: 1.5 pts)

Maps opcode + funct3 (+ funct7 where required) to the correct AluOp.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| E1 | Correct AluOp for all supported instructions; funct7 disambiguates the collisions (ADD/SUB, SRL/SRA, and the M-extension ADD/MUL, SRL/DIVU, etc.); LOAD/STORE forced to ADD; branches use SUB. | -0.0 |
| E2 | Mostly correct, but a minor issue: one funct7 collision resolved wrong (e.g., SRA decoded as SRL), OR the -m disable path not honored (M ops decode even when RV32M is off). | -0.5 |
| E3 | funct7 not consulted where required, so a group of same-funct3 instructions all map to one op, OR LOAD/STORE not forced to ADD. | -1.0 |
| E4 | ALU operation not meaningfully derived from the encoding. | -1.5 |

**Self-assessed step:** E1

**Points claimed:** 1.5

**File(s)/line(s) for verification:**
-
- decode.hpp (Lines 21, 26)
- decode.cpp (Lines 125-266, 286-293)
- hart.hpp (Line 65)
- hart.cpp (Lines 104-109)

**Justification:** I've completed all criteria in this category for earning full points. ALU operations are represented as an enum class. LOAD and STORE instructions are forced to ADD, and branches do use SUB. ADD is sometimes used as a placeholder since the aluop_decode function needs to return either an AluOp enum or a nullopt for invalid decodings, such as in the case of JAL instructions. funct3 is used for OP-IMM and OP to assign an ALU operation from a table, and funct7 is used to handle collisions for OP instructions. The m flag is also used to consider if M-extension instructions should be decoded or not. Any invalid decodings return a nullopt, which will park the HART since it is an illegal instruction (e.g. M ops can't decode when RV32M is off, so no ALU operation is assigned). 


---

## Category F — Special-Case Instructions and PC-Relative Setup (Max: 1.5 pts)

Handles the encoding-specific semantics for LUI, AUIPC, JAL, and JALR.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| F1 | LUI adds the upper immediate to 0; AUIPC adds the upper immediate to the PC of the AUIPC instruction itself; JAL is J-type and PC-relative; JALR is correctly treated as I-type (rs1 + immediate), not J-type. | -0.0 |
| F2 | Three of the four correct, but one minor slip: AUIPC uses the wrong PC (e.g., PC+4 instead of the instruction's own PC), OR LUI adds to something other than 0. | -0.5 |
| F3 | JALR mis-decoded as J-type (or its target computed PC-relative instead of rs1-relative). | -1.0 |
| F4 | Two or more of LUI/AUIPC/JAL/JALR handled incorrectly, OR their PC-relative setup is absent. | -1.5 |

**Self-assessed step:** F1

**Points claimed:** 1.5

**File(s)/line(s) for verification:**
-
- decode.hpp (Line 27)
- decode.cpp (Lines 17-18, 308-332, 346-360)
- pipeline_object.hpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. JALR instructions are assigned the I enum value for I-type instructions in the opcode lookup table. LUI and AUIPC are correctly handled with their special-case values in the operands; PC is correct as it is the PC associated with the instruction on the rising edge thanks to a pipeline object I've built for every HART. JAL is also assigned the J enum type for J-type instructions in the opcode lookup table, and the immediate is stored to be added to the PC later on.


---

## Score Summary

| Category | Max | Earned |
|----------|-----|--------|
| A — Build and Structure | 0.5 | 0.5 |
| B — Opcode Decode & Type Classification | 1.5 | 1.5 |
| C — Field Extraction & Immediates | 2.5 | 2.5 |
| D — Operand Selection & Reg Reads | 2.5 | 2.5 |
| E — ALU Operation Decode | 1.5 | 1.5 |
| F — Special-Case & PC-Relative Setup | 1.5 | 1.5 |
| **Total** | **10.0** | **10.0** |
