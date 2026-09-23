#include "elf.hpp"

size_t read_bytes(std::vector<uint8_t>& buffer, uint32_t offset, uint8_t size) {
    size_t val = 0;

    for (uint8_t i = 0; i < size; i++) {
        val = val | buffer[offset + i] << (i * 8);
    }

    return val;
}

uint32_t load_elf(const std::string& path, Machine& machine) {
    std::ifstream fin;
    std::vector<uint8_t> elf_buffer;
    std::vector<program_headers> ph_store;
    elf_headers headers;
    program_headers ph;

    fin.open(path, std::ios::binary);

    if (!fin.is_open()) {
        fatal_error("Invalid ELF path.");
    }

    fin.seekg(0, std::ios::end);
    std::streamsize size = fin.tellg();
    if (size < 0) {
        fatal_error("Invalid ELF size.");
    }

    fin.seekg(0, std::ios::beg);
    elf_buffer.resize(size);
    fin.read(reinterpret_cast<char*>(elf_buffer.data()), size);

    if (fin.gcount() != size) {
        fatal_error("Short ELF read.");
    }

    headers.e_bitsize = read_bytes(elf_buffer, static_cast<uint32_t>(4), 1);
    headers.e_endian = read_bytes(elf_buffer, static_cast<uint32_t>(5), 1);
    headers.e_filever = read_bytes(elf_buffer, static_cast<uint32_t>(6), 1);
    headers.e_osabi = read_bytes(elf_buffer, static_cast<uint32_t>(7), 1);
    headers.e_abiver = read_bytes(elf_buffer, static_cast<uint32_t>(8), 1);
    headers.e_type = read_bytes(elf_buffer, static_cast<uint32_t>(16), 2);
    headers.e_machine = read_bytes(elf_buffer, static_cast<uint32_t>(18), 2);
    headers.e_version = read_bytes(elf_buffer, static_cast<uint32_t>(20), 4);
    headers.e_entry = read_bytes(elf_buffer, static_cast<uint32_t>(24), 4);
    headers.e_phoff = read_bytes(elf_buffer, static_cast<uint32_t>(28), 4);
    headers.e_shoff = read_bytes(elf_buffer, static_cast<uint32_t>(32), 4);
    headers.e_flags = read_bytes(elf_buffer, static_cast<uint32_t>(36), 4);
    headers.e_ehsize = read_bytes(elf_buffer, static_cast<uint32_t>(40), 2);
    headers.e_phentsize = read_bytes(elf_buffer, static_cast<uint32_t>(42), 2);
    headers.e_phnum = read_bytes(elf_buffer, static_cast<uint32_t>(44), 2);
    headers.e_shentsize = read_bytes(elf_buffer, static_cast<uint32_t>(46), 2);
    headers.e_shnum = read_bytes(elf_buffer, static_cast<uint32_t>(48), 2);
    headers.e_shstrndx = read_bytes(elf_buffer, static_cast<uint32_t>(50), 2);

    if (elf_buffer[0] != 0x7f || elf_buffer[1] != 'E' || elf_buffer[2] != 'L' || elf_buffer[3] != 'F') {
        fatal_error("Invalid ELF magic bytes.");
    } else if (headers.e_type != 2) {
        fatal_error("Non-executable type per ELF.");
    } else if (headers.e_machine != 243) {
        fatal_error("Invalid machine target for ELF.");
    } else if (headers.e_bitsize != 1) {
        fatal_error("Invalid bitsize per ELF.");
    }

    for (uint32_t i = 0; i < headers.e_phnum; i++) {
        ph.p_type = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize), 4);
        ph.p_offset = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 4, 4);
        ph.p_vaddr = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 8, 4);
        ph.p_paddr = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 12, 4);
        ph.p_filesz = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 16, 4);
        ph.p_memsz = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 20, 4);
        ph.p_flags = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 24, 4);
        ph.p_align = read_bytes(elf_buffer, headers.e_phoff + (i * headers.e_phentsize) + 28, 4);

        ph_store.push_back(ph);
    }
}