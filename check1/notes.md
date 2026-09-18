## ISA Project Notes
**Language:** C++

**Version:** Up to 2023

**Extension(s):** ```.cpp```, ```.hpp```

>**NOTE:** Use architecture agnostic data sizes. Since you are using C++, use the standard integers that encode the size in the type, such as ```uint32_t``` and ```int16_t``` in C++.

* Plan to build the multithreaded pipeline on top of these standard primitives only ```<thread>```/ ```<mutex>``` / ```<barrier>```.

>**IMPORTANT! READ THESE CHAPTERS IN THE RISC-V MANUAL**
>* Base Integer Instruction Set
>* "M" Extension
>* "A" Extension
>* "Zicsr" Extension
>* "Zicntr" Extension
>* ```SFENCE.VMA``` Instruction
>* Instruction Set Listings

## Design Decisions

### Machine / Hart split
* `Machine` owns shared state: RAM (sized by `mem` config key), HART count.
* Each `Hart` owns its own per-HART state: register file (x0-x31), PC, CSRs.
* Rationale: registers/PC/CSRs are per-HART per spec, but RAM is shared across
  HARTs. Modeling this now avoids hardcoding a single HART and having to
  rewrite the register file / CSR storage to be HART-indexed later
  (writeup explicitly calls this out as a "wrong turn").

### File layout — fine-grained, one concern per file
* `register_file.hpp/.cpp` — 32-register array, read/write API, x0 rule.
* `hart.hpp/.cpp` — owns a `RegisterFile`, PC, CSRs.
* `machine.hpp/.cpp` — owns RAM + collection of `Hart`s.
* `config.hpp/.cpp` — CLI + config-file parsing and precedence resolution.
* `sign_extend.hpp` — branchless sign-extend function.
* `main.cpp` — entry point, wires config -> Machine.
* Rationale: maps ~1:1 onto rubric categories (B->config, C->machine's RAM,
  D->register_file, E->hart, F->sign_extend), which makes it easy to cite
  exact file/line numbers in RUBRIC.1.md. Matches the writeup's "one concern
  per file" hygiene recommendation.

### Config value representation — hybrid
* Parse CLI + config file into a plain `map<string,string>` (or equivalent)
  to resolve precedence (default -> file -> CLI, last write wins). Keeps
  precedence logic uniform and simple regardless of each switch's real type.
* After precedence is resolved, do one conversion pass that materializes a
  typed `Config` struct (uint32_t mem, bool mmu, enums for cache type, etc).
* Downstream code (Machine, Hart, ...) only ever sees the typed struct, never
  raw strings.
* Scope note: rubric requires ALL ~30 switches in Table 2 to parse correctly
  now (incl. cache/branch-predictor/MMU switches), even though those
  subsystems aren't implemented/graded until later checkpoints.

### Config parsing order (handles the -f chicken-and-egg problem)
* Parse argv ONCE into an intermediate `map<string,string>` of switches the
  user actually typed (no defaults yet).
* Pull `f`, `i`, `o` out of that map directly (CLI-only; `i`/`o` aren't
  precedence keys at all, `f` just locates the config file).
* Build `Config` from hardcoded defaults.
* If `f` was present, parse the file and overlay its values onto `Config`.
* Overlay the intermediate CLI map (minus `f`/`i`/`o`) onto `Config` last --
  this is what guarantees CLI always wins, since it's applied unconditionally
  after the file layer. Avoids parsing argv twice and avoids per-key
  precedence special-casing.

### Suffix-magnitude parsing — two separate helpers
* One helper for binary units (K=1024, M=1024^2, G=1024^3), used by `-mem`
  and `-cS1/2/3`.
* A separate helper for decimal/SI units (K=1000, M=1000^2, G=1000^3), used
  by `-hz`.
* Rationale: `-mem`/cache sizes are binary (kibi/mebi/gibi) but `-hz` is
  frequency, which uses decimal SI prefixes (kilo/mega/giga). A single
  shared helper without a base parameter risks silently getting one of them
  wrong; separating them keeps the unit semantics explicit at the call site.

### Fatal error handling — free function, not exceptions
* A single `fatal(message)`-style free function prints to stderr and calls
  `std::exit()` with a non-zero code. Called from wherever an invalid
  switch/value/key is detected (CLI parsing, file parsing, value
  conversion).
* Rationale: program is terminating anyway, so no RAII/cleanup concern that
  would motivate exceptions. Simpler given rusty C++ (no try/catch needed
  anywhere), and gives one consistent error path across all of config
  parsing.

### RAM allocation — unique_ptr<uint8_t[]> backed by raw new[]
* `std::unique_ptr<uint8_t[]> ram(new uint8_t[size]);` -- RAII cleanup, but
  using `new[]` directly (not `make_unique`) so bytes are left indeterminate
  rather than value-initialized to 0.
* Trap avoided: both `std::vector<uint8_t>(size)` and
  `std::make_unique<uint8_t[]>(size)` zero-initialize, which violates the
  "left uninitialized" requirement (rubric C2).

### Memory access interface — build it now, not later
* `Machine` exposes a `read(addr, size)` / `write(addr, size, val)` API now,
  even though nothing calls it yet this checkpoint (only allocation is
  graded in Category C).
* Rationale: writeup's "Design the Hard-to-Change Things First" -- fetch,
  load, and store all call this later, and cache/MMU will eventually live
  behind it. Building the seam now avoids rewriting every stage later.

### Memory access interface — signature and endianness
* Signature: `uint32_t read(uint32_t addr, uint8_t size)` and
  `void write(uint32_t addr, uint8_t size, uint32_t value)`, where `size` is
  always 1, 2, or 4 bytes.
  * Rationale: RISC-V only ever needs byte/halfword/word access -- no other
    widths exist. One method with a size parameter (vs. three separate
    `readByte`/`readHalf`/`readWord` methods) keeps the call sites in
    fetch/decode/execute uniform, since which width to use is itself a
    per-instruction decode decision (the `memop` field's 0bXX bits) rather
    than something known at compile time. For sizes < 4 bytes, unused upper
    bits of the return value are don't-care -- the caller (decode/execute)
    is responsible for sign- or zero-extending per the specific instruction.
* Endianness: assemble/disassemble multi-byte values with explicit
  shift-and-OR (e.g. `byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24)`
  for little-endian), NOT a pointer cast or memcpy onto `uint32_t`.
  * Rationale: a cast/memcpy approach silently depends on the host machine
    also being little-endian, which is exactly the kind of host-dependent
    "architecture agnostic" violation the writeup warns against elsewhere
    (sign extension, NZCV flags). Explicit shift-and-OR produces the same
    result regardless of host endianness -- correct by construction rather
    than by assumption about the grading machine.

### RegisterFile internals
* Storage: private `std::array<uint32_t, 32>` member. Fixed size known at
  compile time, no heap allocation, standard library only.
* Index 0 gets a real (wasted) slot in the array rather than being
  special-cased out of storage -- simpler 1:1 indexing (`idx` maps directly
  to array index), and 4 bytes is not worth the added complexity of
  excluding it.
* x0 enforcement lives INSIDE `read(idx)`/`write(idx, val)` as a guard
  clause; the backing array stays `private`. This is what makes the rule
  impossible to bypass from a call site (rubric D1 vs. D2 -- D2 dings
  "enforced ad hoc at call sites" as fragile).
* Deliberately NOT implementing the read-on-rising-edge/write-on-falling-edge
  timing or any locking yet -- that's a pipeline/threading concern not
  graded until a later checkpoint. For now `RegisterFile` is a plain,
  synchronous, single-threaded read/write API. Avoids solving a problem this
  checkpoint doesn't have.
* x1-x31 initialized to 0 in the constructor (x0 already reads 0 via the
  API regardless of storage). Spec only mandates x0's behavior -- initial
  values of the rest are genuinely unspecified -- but zero-init buys
  determinism for testing/debugging at no cost, since a well-formed program
  must write a register before relying on its value anyway. Mirrors the
  writeup's own "determinism is a feature" reasoning about the barrier/
  pipeline; NOT the same rule as RAM (Category C requires RAM specifically
  be left uninitialized -- that requirement doesn't extend to registers).

### CSR storage and dispatch (per-Hart)
* Internal storage: 5 fields, not 8 -- `uint32_t mhartid` (RO),
  `uint32_t satp` (RW unless `-mmu off`, then hardwired to 0), and three
  `uint64_t` counters (`cycle`, `time`, `instret`). The `*h` (high 32 bits)
  CSR numbers are just the upper half of the corresponding `uint64_t`,
  split out only at the dispatch boundary -- no separate storage needed
  since software already reads 64-bit CSRs as two instructions.
* Dispatch: a `Csr` class with `read(number)`/`write(number, value)` that
  internally dispatches on CSR number (if-chain/switch/map -- writeup
  allows any). Same "enforce inside the API" pattern as `RegisterFile`'s x0
  rule: read-only CSRs silently reject writes, and `satp` writes are
  ignored (stays 0) whenever the Hart's `-mmu` config is off -- enforced
  inside `write()`, not left to callers to remember.

### Hart <-> Machine wiring
* `Hart` holds a reference (or pointer) to its owning `Machine`, wired once
  at construction.
* Used now to read `machine.getMemorySize()` for initial SP computation;
  same reference will be what fetch/load/store call through in later parts
  (Part III+). One dependency edge instead of wiring it twice.
* Consistent with the earlier decision to build Machine's read/write seam
  now even though nothing calls it yet this checkpoint.

### PC and SP initialization
* PC: a plain private `uint32_t` field directly on `Hart`, with its own
  accessor methods -- NOT part of `RegisterFile`. Rubric calls it
  "non-addressable," i.e. it has no register/CSR number of its own.
* SP (x2) initial value: `mem_size & ~0xF` (mem size rounded down to a
  16-byte boundary), computed in `Hart`'s constructor from
  `machine.getMemorySize()`.
* Initialization goes THROUGH `RegisterFile::write(2, value)`, not by
  poking the backing array directly -- keeps the "one access path to
  registers" guarantee true even at construction time, no exceptions.

## Sign Extension — Conceptual Notes (NOT a solution — mine to derive)

Category F requires a function that takes a value and a sign-bit index and
sign-extends to 32 bits, with NO loops and NO conditionals (if/else/ternary/
switch). This is graded algorithmic logic, not an architecture decision, so
I'm deliberately not writing the formula myself -- only capturing the
conceptual building blocks I worked through, to derive the actual function
on my own.

* **The core identity.** If the value's real bits live in the low
  `k = sign_bit_index + 1` bits, and `U` is those `k` bits read as unsigned,
  then in two's complement the sign bit's positional weight is `-2^(k-1)`
  instead of `+2^(k-1)` -- a difference of exactly `2^k`. So:
  * sign bit 0 -> `V = U` (already correct, no adjustment)
  * sign bit 1 -> `V = U - 2^k` (subtract `2^k` once)
  * The whole problem is just "conditionally subtract `2^k` based on one
    bit, without a conditional."
* **Turning a single bit into a branchless 0-or-`2^k` choice** -- directions
  to explore:
  * Isolate the sign bit (shift + mask) and use it arithmetically --
    multiplying by a 0/1 quantity naturally yields "0 or something" with no
    branch.
  * Broadcast a single bit into an all-zeros/all-ones mask, then combine
    with `AND`/`XOR` against the quantity to conditionally apply.
  * A one-step identity exists that operates directly on the bit pattern
    via XOR + subtract -- worth deriving on paper: for a small `k` (e.g. 4),
    trace what `value ^ mask` does when the top bit is 0 vs. 1, and check
    whether a subsequent subtract fixes up both cases.
* **What to avoid** (rubric F4: "relies on the host to sign-extend"):
  * Don't cast through a smaller *signed* type and back up (lets the
    compiler's own promotion rules do the extension for you).
  * Don't lean on arithmetic-right-shift's sign-bit-replication behavior on
    a signed type -- branchless/loopless in the strict sense, but it
    delegates the actual work to the compiler/hardware instead of
    constructing it myself.
  * Stick to unsigned arithmetic (`uint32_t`) and explicit bitwise ops
    (`&`, `|`, `^`, `<<`, `>>`) plus plain `+`/`-`.
* **Test boundary cases** once I have a candidate:
  * `sign_bit_index = 31` -- should be a no-op (value's already full-width).
  * `sign_bit_index = 11` -- the I-type 12-bit immediate case the writeup
    calls out explicitly.
* Check course lecture notes first -- syllabus allows using the course's
  own notes, just not outside sources (Stack Overflow, Google, ChatGPT).
* **Where it lives**: defined directly in `sign_extend.hpp` as an `inline`
  function -- no `sign_extend.cpp`. It's one small, stateless, pure
  function that many `.cpp` files will `#include` (decode logic later on),
  so a normal (non-inline) definition in the header would cause "multiple
  definition" linker errors once included in more than one translation
  unit. `inline` tells the linker to treat duplicate definitions across
  translation units as one, which is exactly what a header-only function
  needs.
* **Signature/parameter types**: `uint32_t sign_extend(uint32_t value,
  uint8_t sign_bit_index)`.
  * `value` is `uint32_t` -- spec says the function extends the value TO 32
    bits, so a 32-bit container is the natural fit even when the real data
    (e.g. a 12-bit immediate) is narrower.
  * `sign_bit_index` is `uint8_t`, not `int` or `uint32_t` -- "architecture
    agnostic" means fixed/explicit width regardless of host, not "let the
    compiler pick," so plain `int` is out even though the range is small.
    Range is only ever 0-31 (32-bit instructions), so one byte is enough --
    matches the precedent already in the writeup's Instruction Structure
    table, which sizes similarly small-range fields (e.g. `rd`, 0-31) as
    1-byte fields rather than full words.

### bp / tlb storage width — widen past uint8_t
* Writeup gives no explicit upper bound for `-bp` (branch predictor rows)
  or `-tlb` (TLB slots) -- unlike `harts` (1..=16) or `cL`/`bpD` (0..=3).
  `Config` originally stored both as `uint8_t` (0-255), which was an
  accidental side effect of the *defaults* (64, 8) happening to fit in a
  byte, not a deliberate range decision.
* Widening both fields past `uint8_t` (e.g. to `uint32_t`, matching
  `mem`/`hz`) for now, rather than silently capping at 255 with no spec
  basis for that ceiling. A too-large value should fail validation loudly,
  not wrap around silently during string->int conversion.
* Follow-up still needed: `bp`/`tlb`'s validators are currently
  `skip_valid` (accept anything) -- should become "well-formed
  non-negative integer that fits the chosen storage type" at minimum,
  same class of fix already applied to `is_in_range_1_to_16`/`0_to_3`
  (reject non-numeric input instead of crashing on conversion).
* Open question, not yet resolved: whether there's a real intended max.
  Plan is to ask the professor directly if it turns out to matter; this
  note should be updated if/when clarified.

## Build Order (Part I)

Sequenced by dependency graph (what needs what) and by warm-up value, not
strictly rubric-category order:

1. **`sign_extend.hpp`** -- zero dependencies, smallest surface area. Good
   place to shake off C++ rust before anything bigger.
2. **`register_file.hpp`/`.cpp`** -- also zero dependencies, small and
   self-contained. Re-exercises classes/private members/`std::array`/
   bitwise ops before facing harder pieces.
3. **`config.hpp`/`.cpp`** -- no dependency on Machine/Hart, but the
   biggest and most involved piece (also worth the most rubric points).
   Tackle once warmed up, not first.
4. **`machine.hpp`/`.cpp`** -- depends on `Config`'s `mem` field, so comes
   after config.
5. **`hart.hpp`/`.cpp`** -- depends on `Machine` (`getMemorySize()`),
   `RegisterFile`, and the `Csr` class (lives inside hart.hpp/.cpp per the
   E->hart file mapping). Pulls everything else together, so naturally last
   of the core pieces.
6. **`main.cpp`** -- wiring/glue, last. First real smoke test: parse args,
   build a `Machine`, build a `Hart` or two, print register/PC/SP/CSR state
   to eyeball correctness.

Habit: after finishing each of steps 1-5, exercise it standalone (throwaway
scratch `main()`, or manual calls with known inputs/expected outputs) before
building the next piece on top of it. Catching a register-file bug before
it's buried under Hart/Machine logic is cheaper than after.

Reminder: syllabus's one AI exception is generating unit tests (not
implementation code) -- fair game to ask for help with test code at any
point.

