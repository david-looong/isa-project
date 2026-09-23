/* Name: David Long
 * Date: September 23, 2026
 * Synopsis: This CPP file contains the contents of the previously declared functions
 * in elf.hpp.
 */

#include "elf.hpp"

// Reads bytes in little-endian format and returns the value.
size_t read_bytes(std::vector<uint8_t>& buffer, uint32_t offset, uint8_t size) {
    size_t val = 0;

    for (uint8_t i = 0; i < size; i++) {
        val = val | buffer[offset + i] << (i * 8);
    }

    return val;
}

// Loads the ELF file through parsing, validating, and storing the contents of the file.
uint32_t load_elf(const std::string& path, Machine& machine) {
    std::ifstream fin;
    std::vector<uint8_t> elf_buffer;
    std::vector<program_headers> ph_store;
    elf_headers eh;
    program_headers ph;

    // Checks to see if the filepath is valid and can be opened in binary.
    fin.open(path, std::ios::binary);
    if (!fin.is_open()) {
        fatal_error("Invalid ELF path.");
    }

    // Gets the size of the file.
    fin.seekg(0, std::ios::end);
    std::streamsize size = fin.tellg();
    if (size <= 0 || size < 52) {
        fatal_error("Invalid ELF size.");
    }
    fin.seekg(0, std::ios::beg);

    // Resizes the file buffer and reads ELF bytes into the buffer.
    elf_buffer.resize(size);
    fin.read(reinterpret_cast<char*>(elf_buffer.data()), size);

    // Checks to see if file input stream has read all the ELF file contents.
    if (fin.gcount() != size) {
        fatal_error("Short ELF read.");
    }

    // Parses through ELF buffer to assign ELF headers their respective values.
    eh.e_bitsize = read_bytes(elf_buffer, static_cast<uint32_t>(4), 1);
    eh.e_endian = read_bytes(elf_buffer, static_cast<uint32_t>(5), 1);
    eh.e_filever = read_bytes(elf_buffer, static_cast<uint32_t>(6), 1);
    eh.e_osabi = read_bytes(elf_buffer, static_cast<uint32_t>(7), 1);
    eh.e_abiver = read_bytes(elf_buffer, static_cast<uint32_t>(8), 1);
    eh.e_type = read_bytes(elf_buffer, static_cast<uint32_t>(16), 2);
    eh.e_machine = read_bytes(elf_buffer, static_cast<uint32_t>(18), 2);
    eh.e_version = read_bytes(elf_buffer, static_cast<uint32_t>(20), 4);
    eh.e_entry = read_bytes(elf_buffer, static_cast<uint32_t>(24), 4);
    eh.e_phoff = read_bytes(elf_buffer, static_cast<uint32_t>(28), 4);
    eh.e_shoff = read_bytes(elf_buffer, static_cast<uint32_t>(32), 4);
    eh.e_flags = read_bytes(elf_buffer, static_cast<uint32_t>(36), 4);
    eh.e_ehsize = read_bytes(elf_buffer, static_cast<uint32_t>(40), 2);
    eh.e_phentsize = read_bytes(elf_buffer, static_cast<uint32_t>(42), 2);
    eh.e_phnum = read_bytes(elf_buffer, static_cast<uint32_t>(44), 2);
    eh.e_shentsize = read_bytes(elf_buffer, static_cast<uint32_t>(46), 2);
    eh.e_shnum = read_bytes(elf_buffer, static_cast<uint32_t>(48), 2);
    eh.e_shstrndx = read_bytes(elf_buffer, static_cast<uint32_t>(50), 2);

    // Validates that ELF file has correct magic bytes, is an executable type, is specified for
    // RISC-V, and has the correct bitsize (32-bit).
    if (elf_buffer[0] != 0x7f || elf_buffer[1] != 'E' || elf_buffer[2] != 'L' || elf_buffer[3] != 'F') {
        fatal_error("Invalid ELF magic bytes.");
    } else if (eh.e_type != 2) {
        fatal_error("Non-executable type per ELF.");
    } else if (eh.e_machine != 243) {
        fatal_error("Invalid machine target for ELF.");
    } else if (eh.e_bitsize != 1) {
        fatal_error("Invalid bitsize per ELF.");
    }

    // Extra malformed file checking (out-of-bounds entry point, incorrect endian flag, invalid
    // OS target, incorrec header version).
    if (eh.e_entry >= machine.getMemorySize()) {
        fatal_error("Out-of-bounds entry point address.");
    } else if (eh.e_endian != 1) {
        fatal_error("Incorrect endian flag.");
    } else if (eh.e_osabi != 0) {
        fatal_error("Invalid OS target.");
    } else if (eh.e_version != 1) {
        fatal_error("Incorrect header version.");
    }

    // Checks if program header offset is out-of-bounds with program header bytes.
    if (static_cast<uint64_t>(eh.e_phoff) + (eh.e_phnum * eh.e_phentsize) > static_cast<uint64_t>(size)) {
        fatal_error("Out-of-range program header offset.");
    }

    // Parses through all program headers indicated by ELF header e_phnum.
    for (uint32_t i = 0; i < eh.e_phnum; i++) {
        ph.p_type = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize), 4);
        ph.p_offset = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 4, 4);
        ph.p_vaddr = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 8, 4);
        ph.p_paddr = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 12, 4);
        ph.p_filesz = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 16, 4);
        ph.p_memsz = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 20, 4);
        ph.p_flags = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 24, 4);
        ph.p_align = read_bytes(elf_buffer, eh.e_phoff + (i * eh.e_phentsize) + 28, 4);

        // If the type is not a loadable type, skip it.
        if (ph.p_type != 1) continue;

        // Checks to see if virtual address in memory is within range/in-bounds, if program data
        // offset is within bounds, and if file size is greater than memory size.
        if (static_cast<uint64_t>(ph.p_vaddr) + ph.p_memsz > machine.getMemorySize()) {
            fatal_error("Out-of-range virtual address.");
        } else if (static_cast<uint64_t>(ph.p_offset) + ph.p_filesz > static_cast<uint64_t>(size)) {
            fatal_error("Out-of-range program data start.");
        } else if (ph.p_filesz > ph.p_memsz) {
            fatal_error("Program header file size is greater than memory size.");
        }

        // Copies file size amount of bytes into RAM.
        for (uint32_t j = 0; j < ph.p_filesz; j++) {
            machine.write(ph.p_vaddr + j, 1, read_bytes(elf_buffer, ph.p_offset + j, 1));
        }

        // Zero-fills RAM if program header memory size is greater than file size.
        if (ph.p_memsz > ph.p_filesz) {
            for (uint32_t k = 0; k < (ph.p_memsz - ph.p_filesz); k++) {
                machine.write(ph.p_vaddr + ph.p_filesz + k, 1, 0);
            }
        }

        // Stores the loaded program header.
        ph_store.push_back(ph);
    }

    // Returns the memory address of the starting instruction.
    return eh.e_entry;
}