# Build Instructions

## Source files

- `main.cpp`
- `config.cpp`
- `error.cpp` (the `fatal_error` function)
- `machine.cpp`
- `memory_system.cpp` (the memory system: RAM, cache hierarchy, MESI coherency, atomics, MMU/TLB)
- `cache.cpp` (one cache level)
- `register_file.cpp`
- `hart.cpp`
- `elf.cpp` (the ELF loader)
- `branch_predictor.cpp` (the branch predictor table)
- `fetch.cpp` (the fetch stage)
- `decode.cpp` (the decode stage)
- `execute.cpp` (the execute stage)
- `memory.cpp` (the memory stage)
- `writeback.cpp` (the writeback stage, placeholder for Part VII)
- `sign_extend.hpp` (Header, doesn't need to be included in compiling)
- `instruction.hpp` (Header, doesn't need to be included in compiling)
- `pipeline_object.hpp` (Header, doesn't need to be included in compiling)

## Compiling with g++

```
g++ -std=c++23 -Wall -o machine main.cpp config.cpp error.cpp machine.cpp memory_system.cpp cache.cpp register_file.cpp hart.cpp elf.cpp branch_predictor.cpp fetch.cpp decode.cpp execute.cpp memory.cpp writeback.cpp
```

## Compiling with clang++

```
clang++ -std=c++23 -Wall -o machine main.cpp config.cpp error.cpp machine.cpp memory_system.cpp cache.cpp register_file.cpp hart.cpp elf.cpp branch_predictor.cpp fetch.cpp decode.cpp execute.cpp memory.cpp writeback.cpp
```

## Running

The `-i` switch (input ELF file) is required; omitting it is a fatal error.

```
./machine -i <program.elf> [-harts <n>] [-mem <bytes>] [-cL <0-3>] [-cC <none|snoop|dir>] [-mmu <on|off>] [-tlb <slots>] [-tlbE <fifo|lru|lfu>] [other switches...] [-f config.txt]
```

Example:

```
./machine -i stdio.elf -harts 2 -mem 1M -cL 3 -cC snoop -tlb 8
```

## Locking (the writeup's BUILD.md)

The machine is still single-threaded (each stage gets its own thread after Part VII), but the
locks that serialize shared state already exist and are taken in one fixed global order:

1. **Coherence point:** `MemorySystem::bus_mutex` under `-cC snoop`, or the per-granule
   `DirEntry` lock(s) under `-cC dir` (when an access spans two granules, the lower address is
   locked first).
2. **L3** (shared cache): <!-- TODO: name the lock, or note it is covered by (1) for now -->
3. **RAM:** <!-- TODO: name the lock, or note it is covered by (1) for now -->

A coherent access takes its lock(s) right after the fault checks and holds them (as
`std::unique_lock`s) across the bus transaction, the cache read/write, the MESI state update and the
LR/SC reservation updates, so an AMO's read-modify-write is indivisible. `bus_transaction` takes no
lock itself (`std::mutex` is not recursive).

Not yet protected for multithreading (to be added with the threading pass):
- `-cL 0` and `-cC none` take no lock (no RAM mutex yet).
- The `dir` map itself (inserting a new entry).
- Under `-cC dir`, the shared L3 (different granules can reach it concurrently).
