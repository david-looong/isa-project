#ifndef ELF_HPP
#define ELF_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include "machine.hpp"
#include "config.hpp"

struct elf_headers {
    uint32_t e_bitsize;
    uint32_t e_endian;
    uint32_t e_filever;
    uint32_t e_osabi;
    uint32_t e_abiver;
    uint32_t e_type;
    uint32_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint32_t e_ehsize;
    uint32_t e_phentsize;
    uint32_t e_phnum;
    uint32_t e_shentsize;
    uint32_t e_shnum;
    uint32_t e_shstrndx;
};

struct program_headers {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

size_t read_bytes(std::vector<uint8_t>& buffer, uint32_t offset, uint8_t size);
uint32_t load_elf(const std::string& path, Machine& machine);

#endif // ELF_HPP