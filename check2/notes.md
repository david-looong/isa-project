## ISA Project Notes — Part II (ELF Loader)

Continues from `check1/notes.md` (Part I: config, machine memory, register file,
PC/CSRs, sign extension — all complete). Same language/version/extension
constraints apply (C++, up to C++23, `.cpp`/`.hpp`, architecture-agnostic
integer types).

Scope this checkpoint: read a 32-bit RISC-V ELF executable, validate its
header, walk the program header table, copy `PT_LOAD` segments into `Machine`
RAM with zero-fill, and set the entry HART's PC to `e_entry`.

## Rubric/writeup cross-check (done before any design work)

Compared `check2/rubric/rubric_2.html` against the writeup's ELF section
(header table, PH table, "ELF Reading Sequence") field-by-field: offsets,
sizes, the four required header checks (e_ident/e_type/e_machine/e_bitsize),
PH entry size (32 bytes, p_flags at offset 24), PT_LOAD filtering, zero-fill
semantics, and entry-point timing all match exactly. No discrepancies found —
rubric is a straightforward grading translation of the writeup's spec.

Two gaps noted in the *existing Part I code* (not writeup/rubric conflicts,
just things this checkpoint's work needs to close):
* `Hart::pc` is private with no way to set it after construction — the
  loader needs to write `e_entry` into it once loading is done.
* `-i` (input ELF path) is accepted by `parse_cli` but never enforced as
  mandatory, even though the writeup marks it `error` if omitted.

## Design Decisions

### Little-endian field assembly — self-contained helper, not shared with Machine
* The ELF loader reads multi-byte header/PH fields the same way `Machine::read`
  does: bytes into a raw `uint8_t` buffer, then explicit shift-and-OR to
  assemble `uint32_t`/`uint16_t` values. NOT a struct-cast over the raw bytes
  (misaligned per rubric A2, and silently host-endianness-dependent — same
  trap avoided in Part I's `Machine::read`).
* The assembly helper is a small private/free function local to the ELF
  loader's own file, not extracted into a shared helper `Machine` also calls.
  * Rationale: rubric A1 wants the loader self-contained with no external
    dependencies; "reading bytes out of a file buffer" and "accessing VM
    RAM" don't conceptually overlap even though the bit-twiddling looks
    identical, and sharing would mean touching already-submitted/graded
    Part I code for Part II's benefit. A few duplicated lines is cheaper
    than that coupling.

### Reading the ELF file — whole file into one buffer up front
* Open in binary mode and read the entire file into a `std::vector<uint8_t>`
  once. Header fields, PH entries, and segment data are all just offsets
  into that one buffer -- no seekg/tellg bookkeeping, no risk of an
  out-of-sync file cursor across header -> PH table -> segment reads.
* Rationale: test ELF files are small; simplicity beats the (irrelevant at
  this size) memory cost of incremental streaming reads.

### Multi-HART entry point — every HART gets e_entry, not just HART 0 (writeup fact, not a decision)
* Writeup, Multi-HART Bootstrapping: "Your machine has -harts hardware
  threads, and ALL HART's will be set to e_entry which begins the 'HART
  race'... It is up to the software to read mhartid and park all non-0
  HARTs by having each call stop_hart."
* Consequence: the loader/main.cpp must set e_entry on every constructed
  Hart, not just Hart 0. Parking non-boot HARTs is the loaded program's own
  job (via a later-checkpoint stop_hart syscall), not the VM's job at load
  time.

### Entry point reaches Hart::pc via a new Hart::set_pc(uint32_t) method
* Adding a real, general-purpose PC setter now rather than threading
  entry_pc through Hart's constructor.
* Rationale: Part III's fetch stage will need to overwrite/advance PC every
  cycle regardless, so this method is needed soon no matter what -- building
  it now means main.cpp just calls `hart.set_pc(entry_pc)` once per Hart
  after construction, and Part III reuses the same method unmodified rather
  than reworking a constructor-only approach next checkpoint.

### Segment copy / zero-fill — reuse Machine's existing per-byte write(), no new Machine API
* Loop calling the already-existing `Machine::write(addr, 1, byte)` once per
  byte for the `p_filesz` copy, and once per byte (value 0) for the
  `p_memsz - p_filesz` zero-fill remainder.
* Rationale: no new surface area on an already-graded Part I class; segment
  sizes in the test ELF files are small enough that a byte-at-a-time loop
  isn't a real perf concern. Consistent with "loader depends on Machine's
  existing public API only."

### Error handling — reuse config.cpp's fatal_error(), one error path for the whole program
* Every ELF validation failure (bad magic/type/machine/bitsize, malformed PH
  table, out-of-bounds segment) calls the same `fatal_error(message)` used
  throughout Part I's config parsing (print to stderr, `exit(1)`).
* Rationale: a controlled `exit(1)` is not the kind of crash rubric F1 warns
  against (that's segfault/UB) -- it's the intended "reject gracefully"
  behavior. One consistent error path beats a second, differently-shaped
  return-code convention just for this file. Means `elf.cpp` includes
  `config.hpp` for this one declaration -- an internal project dependency,
  not the kind of external/third-party dependency rubric A1 cares about.

### Out-of-bounds segment (p_vaddr + p_memsz > RAM size) — reject the whole file
* Treated as fatal, same tier as a bad magic number or wrong e_machine --
  stops before loading any further segments, rather than partially loading
  the program and continuing.
* Rationale: a program missing data it expected to have is arguably more
  dangerous to run than refusing to run it at all; matches typical loader
  semantics and rubric F1's framing of bounds violations alongside the
  other "reject cleanly" checks.

### Loader interface — load_elf(path, Machine&) returns e_entry
* One free function in `elf.hpp`/`elf.cpp`: `uint32_t load_elf(const
  std::string& path, Machine& machine)`. Does all header/PH validation and
  segment copying internally; returns `e_entry` once done.
* Called once from `main.cpp`, after `Machine` is constructed but before the
  `Hart`-construction loop; the returned value is passed to every
  constructed `Hart` via `set_pc()` (per the "ALL HARTs get e_entry" fact
  above).
* Rationale: keeps the loader's only project-level dependencies as `Machine`
  (to write segments) and `fatal_error` (from `config.hpp`, for error
  reporting) -- it doesn't need to know `Hart`/`RegisterFile` exist at all.
  Also keeps `main.cpp`'s existing Machine-then-Harts construction order
  intact (loading slots in as one step between them) rather than requiring
  Harts to exist before loading can happen.

### Mandatory -i check — lives in main.cpp, mirrors the existing -f pattern
* `main.cpp` checks `cli_map.find("i") == cli_map.end()` and calls
  `fatal_error(...)` if missing, the required-instead-of-optional twin of
  the existing `if (cli_map.find("f") != cli_map.end())` check already
  there for the config file.
* Rationale: the loader should only ever be called with a path already
  known to exist in `cli_map` -- keeps "is -i present" a CLI-layer concern,
  not something `load_elf`/`elf.cpp` has to re-check.

### PH table stride — use e_phentsize from the file, not a hardcoded 32
* The loop over program headers advances by `e_phentsize` bytes (read out
  of the ELF header) per entry, even though for 32-bit ELF it will always
  actually be 32.
* Rationale: rubric C2 explicitly dings "uses a hardcoded entry size instead
  of e_phentsize/32 where it matters" -- costs nothing to read the field and
  use it as the stride instead of assuming its value.

### Header field offsets — named constants + a small struct, NOT a config-style lookup table
* Considered mirroring `config.cpp`'s `switch_table_validator`/`switch_table_modifier`
  (string key -> behavior) for the ELF header fields, but that pattern solves
  a different problem: config's switches are arbitrary keys the *user*
  supplies at runtime, so a string-keyed table is what lets ~30 of them be
  handled uniformly. The ELF header's layout is fixed at compile time --
  `e_entry` is always offset 24, `e_machine` always offset 18, per the
  writeup's own Table 3 -- there's no dynamic dispatch happening, so a
  `map<string, size>` would add indirection without solving anything, and
  would reintroduce exactly the "get the cumulative offset wrong" bug class
  the writeup's table already hands you the answer to avoid.
* Instead: named `constexpr` offset constants (matching Table 3's offsets
  literally, e.g. an `E_ENTRY_OFFSET = 24`), used to populate a small struct
  (`e_bitsize`, `e_type`, `e_machine`, `e_entry`, `e_phoff`, `e_phentsize`,
  `e_phnum`, plus `e_ident` handled as raw bytes) via one call per field to
  the little-endian read helper. Only ~8 of the header's ~19 fields are
  actually needed -- the rest (`e_shoff`, `e_flags`, `e_shentsize`,
  `e_shnum`, `e_shstrndx`, etc.) are out of scope since section headers
  aren't used.

## Build Order (Part II)

Sequenced by dependency graph, same principle as Part I -- smallest/lowest-risk
piece first, biggest/most-involved piece once warmed back up, wiring last.

1. **`Hart::set_pc(uint32_t)`** (in `hart.hpp`/`hart.cpp`) -- small, isolated
   addition to an already-working class, zero new files, zero new
   dependencies. Good warm-up before the bigger piece, and unblocks nothing
   else being risky to get wrong.
2. **`elf.hpp`/`elf.cpp`** -- the actual loader (`load_elf`). Depends only on
   `Machine` (existing `write()`/`getMemorySize()`) and `config.hpp`
   (`fatal_error`) -- no dependency on `Hart` at all per the interface
   decision above, so this can be built and exercised standalone before
   touching `main.cpp`. By far the biggest and most point-heavy piece
   (6.5 of 10 rubric points across Categories B-D), so most of the time
   goes here.
3. **`main.cpp` wiring** -- last: add the mandatory `-i` check, call
   `load_elf()` once between constructing `Machine` and the `Hart`-
   construction loop, call `hart.set_pc(entry_pc)` on every constructed
   HART. Pure glue over the two pieces above, so it should be quick once
   they're both solid.

Habit (same as Part I): after step 1 and step 2, exercise each standalone
(throwaway scratch `main()`, manual calls with known inputs/expected
outputs) before building the next piece on top of it -- catching a bug in
`load_elf` before it's buried under `main.cpp` wiring is cheaper than after.

Testing note: standalone-testing `elf.cpp` needs at least one real 32-bit
RISC-V ELF file (e.g. something like the writeup's `stdio.elf`) plus ideally
the instructor's `viewelf` dump to diff against -- neither is in this repo
yet. Worth tracking down before starting step 2, or a hand-built minimal
ELF (valid header + one tiny PT_LOAD segment) can stand in for the header/PH
parsing checks in the meantime, with correctness against the real test files
verified once available.

Reminder: syllabus's one AI exception is generating unit tests (not
implementation code) -- fair game to ask for help with test code at any
point, same as Part I.

## Implementation log (what was actually built, and where it differed from the plan above)

Build order steps 1-3 are done: `Hart::set_pc`, `elf.hpp`/`elf.cpp`, and the
`main.cpp` wiring. Verified end-to-end: memory image from `load_elf` matched the
instructor's `viewelf` dump byte-for-byte on `stdio.elf` (run on the Tesla
machine), and every hand-built test ELF behaves as expected.

### Deviations from the design decisions above
* **No named offset constants.** Kept the small `elf_headers`/`program_headers`
  structs, but dropped the `constexpr` offset names and pass literal offsets to
  `read_bytes` (file-length tradeoff). `e_padding` was dropped from the struct
  entirely -- unused, and a 7-byte read can't fit `read_bytes`' 4-byte contract.
* **`read_bytes(buffer, offset, size)`** is the shift-and-OR helper: returns
  `size_t`, takes the buffer by reference, `offset` is `uint32_t` (a `uint8_t`
  offset would wrap past byte 255), and `size` must stay <= 4.
* **`e_ident` is compared as four raw bytes**, not run through `read_bytes`.
  A first version special-cased `offset == 0` to reverse byte order; removed.

### Bugs found and fixed while building (lessons worth keeping)
* **Struct field widths must match the field's on-disk size.** `uint8_t`
  fields silently truncated `e_entry` (0x10000 -> 0); `uint16_t` PH fields did
  the same to `p_vaddr`. All fields are now `uint32_t`.
* **Integer promotion defeats a wider destination.** `byte << (i*8)` promotes
  the byte to `int` before shifting, so widening `val` did not fix the shift UB.
* **`seekg(std::ios::end)` (one argument) is an absolute seek to offset 2**, not
  "end of file". Needs `seekg(0, std::ios::end)`. `tellg()` result is held in a
  `std::streamsize` and checked `< 0` (a `uint32_t` would wrap -1 to ~4 GiB).
* **`&&` vs `||` on the magic-byte check.** "Reject if ANY byte is wrong" needs `||`.
* **Non-`PT_LOAD` segments are skipped (`continue`), never fatal.** Real ELF files
  always contain some (`PT_NOTE`, `PT_GNU_STACK`, ...).
* **Bounds check must not overflow.** `p_vaddr + p_memsz` in `uint32_t` wrapped for
  `p_vaddr = 0xFFFFFFF0` and slipped past the check; the sum is computed in
  `uint64_t`.
* **Nested loop variable shadowing** (`i` reused inside the segment-copy loop).
* **`main.cpp` regression:** moving the `-i` check ahead of the CLI-overlay loop
  made every run fail, because `-i` is CLI-only and `resolved` doesn't contain it
  until the overlay runs. The check must come after the overlay (or test `cli_map`).

### RESOLVED: malformed-file robustness (rubric F1 vs F3)
`load_elf` originally indexed `elf_buffer` without checking offsets against the
file's real size, so empty/short files, a truncated PH table, `e_phoff` past EOF,
and segments whose data ran past EOF all crashed with out-of-bounds reads. Now
rejected via `fatal_error`, along with the checks below, all verified under
ASan/UBSan against hand-built malformed ELFs:
* file size < 52 bytes (also covers empty file);
* `e_phoff + e_phnum * e_phentsize` past end of file;
* `p_offset + p_filesz` past end of file (sum in `uint64_t`, no wraparound);
* `p_vaddr + p_memsz` past RAM (sum in `uint64_t`);
* `p_filesz > p_memsz` (previously let the copy loop write past the end of RAM
  even though the `p_memsz`-based bounds check passed);
* `e_entry >= RAM size` (an entry equal to the RAM size is one past the end);
* `e_endian != 1`, `e_osabi != 0`, `e_version != 1` (writeup: "must be").
Sign-compare warnings on the size comparisons fixed (GCC includes
`-Wsign-compare` in `-Wall`; clang only in `-Wextra`).

Known and accepted: `e_phentsize` is used as the PH stride but not validated
`>= 32`; a crafted file with `e_phentsize` of 0-31 near end of file still reads
out of bounds. TA confirmed it is always 32 in the test files, so the check was
deliberately left out.

Re-verified on the Tesla machine after the final checks: `stdio.elf` still
matches the instructor's `viewelf` memory dump, and the build is clean with
`-Wall`.

### Testing artifacts (NOT part of the submission; removed from the repo)
Hand-built ELF fixtures (`test_valid.elf`, `test_bad_*.elf`, `test_oob_segment.elf`,
`test_overflow_segment.elf`, the malformed/adversarial `test_*` set) and
`dump_elf_memory.cpp` (dumps `Machine` RAM after `load_elf`, for diffing against
`viewelf`) were used for verification and then taken out of the repo. The writeup
says to submit only the files for this part and remove debugging code.
