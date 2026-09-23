# Build Instructions

## Source files

- `main.cpp`
- `config.cpp`
- `machine.cpp`
- `register_file.cpp`
- `hart.cpp`
- `elf.cpp` (the ELF loader)
- `sign_extend.hpp` (Header, doesn't need to be included in compiling)

## Compiling with g++

```
g++ -std=c++23 -Wall -o machine main.cpp config.cpp machine.cpp register_file.cpp hart.cpp elf.cpp
```

## Compiling with clang++

```
clang++ -std=c++23 -Wall -o machine main.cpp config.cpp machine.cpp register_file.cpp hart.cpp elf.cpp
```

## Running

The `-i` switch (input ELF file) is required; omitting it is a fatal error.

```
./machine -i <program.elf> [-harts <n>] [-mem <bytes>] [other switches...] [-f config.txt]
```

Example:

```
./machine -i stdio.elf -harts 2 -mem 1M
```
