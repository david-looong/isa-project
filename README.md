# RISCV32IMA ISA Project
This project is an implementation of a virtual machine (VM) that emulates the RISCV32IMA instruction set architecture (ISA). This project is a semester-long course project for the Computer Systems Organization (COSC 530) course at the University of Tennessee, Knoxville.

## Introduction
The RISCV32IMA(Zicsr)(Zicntr) specification means:
1. **RV32** - It is the RISCV 32-bit instruction specification.
2. **RV32I** - It includes all integer instructions.
3. **RV32M** - It includes multiplication and division instructions.
4. **RV32A** - It includes atomic memory operation (AMO) instructions.
    * **Alias** - RV32A implies two extensions: (1) ```Zaamo``` and (2) ```Zalrsc```.
    * **Zaamo** - It includes the following atomic instructions: ```amoswap```, ```amoadd```, ```amoand```, ```amoor```, ```amoxor```, ```amomax[u]```, ```amomin[u]```.
    * **Zalrsc** - It includes Load-Reserved and Store Conditional ```LR``` and ```SC```.
5. **Zicsr** - It includes *some* control and status registers (CSR), but it includes all six CSR instructions (```csrrw```, ```csrrs```, ```csrrc```, ```csrrwi```, ```csrrsi```, and ```csrrci```).
6. **Zicntr** - It supports base counters and timers.

The VM reads an ELF file and goes through the instruction pipeline stages (fetch, decode, execute, memory, writeback) for each instruction. Each one of the five stages of the pipeline is in its own thread.

## Language
This project is written in C++23 and does not contain any external dependencies. The project also utilizes architecture agnostic data sizes, so there are no types whose size depend on the architecture present in this project. Instead, standard integers (e.g., ```uint32_t```) are used in place of these types.

## Contents
This repository contains all source files required to run the project as well as seven checkpoint folders. This project was graded in seven checkpoints as well as an overall final submission, so each checkpoint serves as a timestamp of where the project currently was at the time of submitting for that checkpoint. Each checkpoint folder includes the following:
* ```README.X.md``` - Build instructions on how to compile and run that version of the project. Compiling instructions are provided for both g++ and clang++. It also includes the source files for the project at the time of the checkpoint.
* ```RUBRIC.X.md``` - The rubric for that particular checkpoint. It contains all the graded categories and subcategories that determines how many points would be earned. Per submission instructions, this file serves as a means for justifying why I believe I satisfy the specific criteria for each rubric category by citing the source files and code that accounts for it and providing a verbal explanation of what the code does.
* ```check_X.zip``` - A zip folder that contains the source files of my project at the time of submitting for a particular checkpoint. Naturally, the contents of the folder grow with each checkpoint.

## Build Instructions
TBD!

## AI Disclosure
I utilized two models from Anthropic via Claude Code to assist me throughout this project: Sonnet 5 and Opus 5.5. These models were only used for having high-level discussions on design decisions and generating test case suites for each checkpoint.