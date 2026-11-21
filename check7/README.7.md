# Build Instructions

## Source files

- `main.cpp` (startup and the clock loop)
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
- `writeback.cpp` (the writeback stage)
- `syscall.cpp` (system call dispatch and the HART start/stop bookkeeping)
- `sign_extend.hpp` (Header, doesn't need to be included in compiling)
- `instruction.hpp` (Header, doesn't need to be included in compiling)
- `pipeline_object.hpp` (Header, doesn't need to be included in compiling)

## Compiling with g++

```
g++ -std=c++23 -Wall -o machine main.cpp config.cpp error.cpp machine.cpp memory_system.cpp cache.cpp register_file.cpp hart.cpp elf.cpp branch_predictor.cpp fetch.cpp decode.cpp execute.cpp memory.cpp writeback.cpp syscall.cpp
```

## Compiling with clang++

```
clang++ -std=c++23 -Wall -o machine main.cpp config.cpp error.cpp machine.cpp memory_system.cpp cache.cpp register_file.cpp hart.cpp elf.cpp branch_predictor.cpp fetch.cpp decode.cpp execute.cpp memory.cpp writeback.cpp syscall.cpp
```

**Platform note:** the `getchar` system call must not block (writeup Table 8), which standard C++
cannot do on standard input. It uses POSIX `poll()` and `read()` (`<poll.h>`, `<unistd.h>`), so the
machine builds on Linux and macOS, but not on Windows.

## Running

The `-i` switch (input ELF file) is required; omitting it is a fatal error.

```
./machine -i <program.elf> [-harts <n>] [-mem <bytes>] [-hz <rate>] [-o <file|->] [-cL <0-3>] [-cC <none|snoop|dir>] [-mmu <on|off>] [-tlb <slots>] [other switches...] [-f config.txt]
```

Example:

```
./machine -i stdio.elf -harts 2 -mem 1M
```

Standard output contains only what the program writes with `putchar`; `getchar` reads standard
input (redirect a file with `< input.txt`).

- **`-o <file>`** sends the `putchar` output to that file instead (created or truncated at
  startup; a path that can't be opened is a fatal error). `-o -`, the default, is standard output.
- **`-hz <rate>`** is the clock rate in ticks per second (decimal suffixes: `1K` = 1,000, `1M`,
  `1G`; 1 to 4,294,967,295). The machine runs at the slower of that rate and the host's speed.
  The default is 100 Hz, so a program that needs many ticks finishes much sooner with a high
  rate, for example `-hz 1G` (effectively unpaced).

The machine runs until:

- a HART calls `exit`: the host exit code is the program's a0 (the host keeps its low 8 bits);
- every HART has stopped with `stop_hart`: exit code 0;
- every HART is parked and at least one was parked by a fault: exit code 1.