# Build Instructions

## Source files

- `main.cpp`
- `config.cpp`
- `machine.cpp`
- `register_file.cpp`
- `hart.cpp`
- `sign_extend.hpp` (Header, doesn't need to be included in compiling)

## Compiling with g++

```
g++ -std=c++23 -Wall -o machine main.cpp config.cpp machine.cpp register_file.cpp hart.cpp
```

## Compiling with clang++

```
clang++ -std=c++23 -Wall -o machine main.cpp config.cpp machine.cpp register_file.cpp hart.cpp
```

## Expected output

For each configured HART, the program prints that HART's full register
file (`x0`-`x31`), program counter, and all six CSRs (`mhartid`, `satp`,
`time`/`timeh`, `cycle`/`cycleh`, `instret`/`instreth`), separated by a
banner per HART.
