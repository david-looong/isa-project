# RUBRIC.2.md — Part II Self-Assessment

**Name:** David Long

**Date:** 09/23/2026



---

## Category A — Build and Structure (Max: 0.5 pts)

Does the Part II code compile and is the ELF loader factored into its own source file(s) as required?

| Step | Criterion | Deduction |
|------|-----------|-----------|
| A1 | Compiles cleanly, ELF loader in its own file, no external dependencies. | -0.0 |
| A2 | Compiles but emits warnings that indicate real problems (e.g., truncation, signedness, unaligned reads on header fields). | -0.25 |
| A3 | Compiles only after trivial fixes (missing include/import, one typo), OR loader is not separated into its own file. | -0.5 |
| A4 | Does not compile; requires nontrivial edits to build. | -0.5 (and verify remaining categories by code reading only) |

**Self-assessed step:** A1

**Points claimed:** 0.5

**File(s)/line(s) for verification:**
-
- Build.md
- elf.hpp (Lines 19-50)
- elf.cpp (Entire file)

**Justification:** I've completed all criteria in this category for earning full points. If you refer to my Build.md file, there are instructions in there for how to compile all my files and run it in order to print out some information. My ELF loader, which I implemented in a function, can be found in elf.hpp/.cpp, separate from the rest of the code. Ever header used is from the C++ standard library.


---

## Category B — ELF Header Parsing and Validation (Max: 2.5 pts)

Correctly reads the 52-byte ELF header at the required field offsets and performs the four required verification checks before loading.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| B1 | All header fields read at correct offsets/sizes; validates e_ident (\x7fELF), e_type (executable), e_machine (RISC-V = 243), and e_bitsize (32-bit); rejects invalid files gracefully. | -0.0 |
| B2 | Header parsed correctly, but one of the four validation checks is missing or too weak (e.g., checks magic but not e_machine). | -0.5 |
| B3 | Two or more validation checks missing, OR one header field read at a wrong offset/size that happens not to break the test files. | -1.0 |
| B4 | Header field offsets are systematically wrong (e.g., 64-bit layout assumed), OR validation is essentially absent (loads anything). | -1.75 |
| B5 | Header not meaningfully parsed; e_entry/e_phoff/e_phnum not obtained correctly. | -2.5 |

**Self-assessed step:** B1

**Points claimed:** 2.5

**File(s)/line(s) for verification:**
-
- elf.hpp (Lines 19-36)
- elf.cpp (Lines 51-79)

**Justification:** I've completed all criteria in this category for earning full points. All header bytes are read with the correct offsets and sizes from a buffer I stored the ELF's contents in into their respective field members of a elf_headers struct. I have checks making sure the ELF magic bytes are valid, the e_type is an executable, the machine target is RISC-V, and that e_bitsize is 32-bit. If any of these checks fail, it calls the fatal_error function from config.hpp/.cpp, which prints out an error message and exits.


---

## Category C — Program Header Table Parsing (Max: 2.0 pts)

Seeks to e_phoff, iterates e_phnum entries of 32 bytes each, and reads the program header fields at the correct offsets.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| C1 | Seeks to e_phoff; iterates exactly e_phnum entries of e_phentsize/32 bytes; all PH fields read at correct offsets (note p_flags at offset 24, unlike 64-bit ELF); filters on p_type == PT_LOAD. | -0.0 |
| C2 | Iterates correctly but one PH field read at a wrong offset, OR uses a hardcoded entry size instead of e_phentsize/32 where it matters. | -0.5 |
| C3 | Does not filter on p_type (attempts to load non-loadable segments), OR miscounts entries (off-by-one on e_phnum). | -1.0 |
| C4 | Program header table located or iterated incorrectly (wrong seek base, wrong stride) so segments are read from the wrong offsets. | -1.5 |
| C5 | Program header table not parsed. | -2.0 |

**Self-assessed step:** C1

**Points claimed:** 2.0

**File(s)/line(s) for verification:**
-
- elf.hpp (Lines 38-47)
- elf.cpp (Lines 98-110)

**Justification:** I've completed all criteria in this category for earning full points. I have a for loop iterating through the value of e_phnum (the number of program headers), and filling field members of the program_headers struct with their appropriate bytes from the elf_buffer. The offset is calculated by adding the program header offset to the program header entry size (which is always 32) multiplied by what number program header the for loop is currently looking at, finally topped by adding the program header relative offset. There is a check for the p_type not being PT_LOAD, in which case it continues to the next program header without storing/loading the current program header.


---

## Category D — Segment Loading and Zero-Fill (Max: 3.0 pts)

Copies each loadable segment's p_filesz bytes from file offset p_offset to memory address p_vaddr, and zero-fills the p_memsz - p_filesz remainder.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| D1 | Copies exactly p_filesz bytes from p_offset to p_vaddr; zero-fills p_memsz - p_filesz trailing bytes; handles multiple loadable segments in order. | -0.0 |
| D2 | Correct copy and zero-fill, but a minor issue: does not handle multiple segments robustly, or assumes zeroed memory instead of explicitly zero-filling (works only if RAM starts zeroed). | -1.0 |
| D3 | Loads a valid ELF but silently mishandles p_memsz > p_filesz (no zero-fill, or .bss left as garbage). | -2.0 |
| D4 | Copies p_memsz bytes from the file (reads past on-disk segment data, corrupting memory), OR copies to p_offset/p_paddr instead of p_vaddr. | -2.5 |
| D5 | Segment data not copied into memory correctly (wrong source, wrong destination, or not at all). | -3.0 |

**Self-assessed step:** D1

**Points claimed:** 3.0

**File(s)/line(s) for verification:**
-
- elf.cpp (Lines 122-132)

**Justification:** I've completed all criteria in this category for earning full points. I iterate over the file size and write byte-by-byte (starting from the program data offset) to RAM indexed at the virtual address for that program header. Afterwards, I check if the memory size if greater than the file size and if so, then to zero-fill the rest of RAM from index virtual_address + file_size to index virtual_address + mem_size. Since the outer for loop is looping through all program headers sequentially, each loadable segment is handled in order.


---

## Category E — Entry Point (Max: 1.0 pts)

Sets the program counter to e_entry after loading.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| E1 | PC initialized to e_entry from the ELF header after all segments are loaded. | -0.0 |
| E2 | Entry point obtained but applied at the wrong time (e.g., before loading) or to the wrong register/field. | -0.5 |
| E3 | Entry point not set, OR hardcoded to a fixed address instead of e_entry. | -1.0 |

**Self-assessed step:** E1

**Points claimed:** 1.0

**File(s)/line(s) for verification:**
-
- main.cpp (Lines 45-46, 52)
- hart.hpp (Line 51)
- hart.cpp (Lines 58-62)

**Justification:** I've completed all criteria in this category for earning full points. load_elf returns a uint32_t (i.e., the e_entry address), which only occurs after every segment has been loaded. After obtaining that in the driver code, a Hart object will call a setter function for the program counter and pass in the entry address, setting the program counter equal to that address.


---

## Category F — Robustness and Endianness (Max: 1.0 pts)

Multi-byte fields treated as little-endian; loader fails safely on malformed input rather than crashing.

| Step | Criterion | Deduction |
|------|-----------|-----------|
| F1 | All multi-byte header/PH fields interpreted little-endian; malformed or non-conforming files are rejected with a clear error, no crash or out-of-bounds access. | -0.0 |
| F2 | Little-endian handling correct, but bounds/error handling is weak (e.g., a segment whose p_vaddr + p_memsz exceeds RAM is not caught). | -0.5 |
| F3 | Endianness assumed from the host without translation (works only on little-endian hosts), OR malformed input causes a crash/undefined behavior. | -1.0 |

**Self-assessed step:** F1

**Points claimed:** 1.0

**File(s)/line(s) for verification:**
-
- elf.hpp (Line 50)
- elf.cpp (Lines 9-18, 69-96, 112-120)

**Justification:** I've completed all criteria in this category for earning full points. My read_bytes helper function interprets the bytes in the buffer as little-endian via shift-and-OR before returning the bytes associated with their respective header/PH fields. There are many checks to account for malformed files to ensure no crashes or out-of-bounds accesses occur (ELF magic bytes, correct e_type, RISC-V target, 32-bit bitsize, entry point address in bounds, little endian flag, valid OS target, valid header version, program header offset in bounds, virtual address in bounds, program data start in bounds, program header file being less than memory size).


---

## Score Summary

| Category | Max | Earned |
|----------|-----|--------|
| A — Build and Structure | 0.5 | 0.5 |
| B — ELF Header Parsing | 2.5 | 2.5 |
| C — Program Header Parsing | 2.0 | 2.0 |
| D — Segment Loading & Zero-Fill | 3.0 | 3.0 |
| E — Entry Point | 1.0 | 1.0 |
| F — Robustness & Endianness | 1.0 | 1.0 |
| **Total** | **10.0** | **10.0** |
