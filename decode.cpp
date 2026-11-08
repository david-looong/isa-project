/* Name: David Long
 * Date: October 21, 2026
 * Synopsis: This CPP files contains the contents of the previously declared functions
 * in decode.hpp as well as an opcode lookup table for assigning instruction types.
 */

#include "decode.hpp"

// Constructs an opcode lookup table for acceptable/illegal instruction types.
std::array<InstType, 128> build_opcode_table() {
    std::array<InstType, 128> table;
    std::fill(std::begin(table), std::end(table), InstType::Illegal);

    table[0b0000011] = InstType::I;
    table[0b0100011] = InstType::S;
    table[0b1100011] = InstType::B;
    table[0b1100111] = InstType::I;
    table[0b1101111] = InstType::J;
    table[0b0010011] = InstType::I;
    table[0b0110011] = InstType::R;
    table[0b1110011] = InstType::I;
    table[0b0010111] = InstType::U;
    table[0b0110111] = InstType::U;
    table[0b0101111] = InstType::R;

    return table;
}

// Initializes the opcode lookup table.
const std::array<InstType, 128> opcode_table = build_opcode_table();

// Returns the instruction type of the opcode based on the opcode lookup table.
InstType classify_opcode(uint8_t opcode) {
    return opcode_table[opcode & 0x7F];
}

// Extracts different field members based on the instruction type.
std::tuple<uint32_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t> field_extraction(uint32_t inst, InstType type) {
    uint32_t immediate = 0;
    uint8_t sign_bit_index = 0;
    uint8_t rd = 0;
    uint8_t funct3 = 0;
    uint8_t rs1 = 0;
    uint8_t rs2 = 0;
    uint8_t funct7 = 0;

    switch (type) {
        // R-type instruction field extraction.
        case InstType::R:
            immediate = 0;
            sign_bit_index = 0;
            rd = (inst & 0xF80) >> 7;
            funct3 = (inst & 0x7000) >> 12;
            rs1 = (inst & 0xF8000) >> 15;
            rs2 = (inst & 0x1F00000) >> 20;
            funct7 = (inst & 0xFE000000) >> 25;
            break;
        // I-type field extraction. If the opcode is a SYSTEM opcode, the field
        // extraction resembles that more of an R-type instruction.
        case InstType::I:
            rd = (inst & 0xF80) >> 7;
            funct3 = (inst & 0x7000) >> 12;
            rs1 = (inst & 0xF8000) >> 15;

            if ((inst & 0x7F) == 0b1110011) {
                immediate = (inst & 0xFFF00000) >> 20;
                sign_bit_index = 31;
                rs2 = (inst & 0x1F00000) >> 20;
                funct7 = (inst & 0xFE000000) >> 25;
            } else {
                immediate = (inst & 0xFFF00000) >> 20;
                sign_bit_index = 11;
                rs2 = 0;
                funct7 = 0;
            }
            break;
        // S-type instruction field extraction.
        case InstType::S:
            immediate = ((inst & 0xFE000000) >> 20) | ((inst & 0xF80) >> 7);
            sign_bit_index = 11;
            rd = 0;
            funct3 = (inst & 0x7000) >> 12;
            rs1 = (inst & 0xF8000) >> 15;
            rs2 = (inst & 0x1F00000) >> 20;
            funct7 = 0;
            break;
        // B-type instruction field extraction.
        case InstType::B:
            immediate = ((inst & 0x80000000) >> 19) | ((inst & 0x7E000000) >> 20) | ((inst & 0xF00) >> 7) | ((inst & 0x80) << 4);
            sign_bit_index = 12;
            rd = 0;
            funct3 = (inst & 0x7000) >> 12;
            rs1 = (inst & 0xF8000) >> 15;
            rs2 = (inst & 0x1F00000) >> 20;
            funct7 = 0;
            break;
        // U-type instruction field extraction.
        case InstType::U:
            immediate = inst & 0xFFFFF000;
            sign_bit_index = 31;
            rd = (inst & 0xF80) >> 7;
            funct3 = 0;
            rs1 = 0;
            rs2 = 0;
            funct7 = 0;
            break;
        // J-type instruction field extraction
        case InstType::J:
            immediate = ((inst & 0x80000000) >> 11) | ((inst & 0x7FE00000) >> 20) | ((inst & 0x100000) >> 9) | (inst & 0xFF000);
            sign_bit_index = 20;
            rd = (inst & 0xF80) >> 7;
            funct3 = 0;
            rs1 = 0;
            rs2 = 0;
            funct7 = 0;
            break;
        default:
            break;
    }

    // Returns the extracted fields, including the sign-extended immediate value.
    return {sign_extend(immediate, sign_bit_index), rd, funct3, rs1, rs2, funct7};
}

// Returns the ALU operation associated with the opcode plus the extracted field values.
std::optional<AluOp> aluop_decode(uint8_t opcode, uint8_t funct3, uint8_t funct7, uint8_t rd, uint8_t rs1, uint8_t srai_bit, uint32_t immediate, bool m, bool a) {
    switch (opcode) {
        // LOAD instructions are forced to ADD.
        case 0b0000011:
            if (funct3 == 0b011 || funct3 == 0b110 || funct3 == 0b111)  return std::nullopt; 
            else                                                        return AluOp::Add;
        // STORE instructions are forced to ADD.
        case 0b0100011:
            if (funct3 > 0b010) return std::nullopt;
            else                return AluOp::Add;
        // BRANCH instructions are forced to SUB.
        case 0b1100011:
            if (funct3 != 0b010 && funct3 != 0b011) return AluOp::Sub;
            else                                    return std::nullopt;
        // JALR instructions are treated as I-type instructions, so they use ADD.
        case 0b1100111:
            if (funct3 == 0b000)    return AluOp::Add;
            else                    return std::nullopt;
        // JAL instructions don't use the ALU, so ADD is a placeholder.
        case 0b1101111:
            return AluOp::Add;
        // OP_IMM instructions have a variety of different ALU operations based on
        // the funct3 value.
        case 0b0010011: {
            // ADDI, SLLI, SLTI, SLTIU, XORI, SRLI, ORI, ANDI
            std::array<std::optional<AluOp>, 8> aluop_imm_table = {
                AluOp::Add, AluOp::LeftShift, AluOp::Sub, AluOp::Sub,
                AluOp::Xor, AluOp::RightShiftL, AluOp::Or, AluOp::And
            };
            std::optional<AluOp> aluop_imm = aluop_imm_table[funct3];

            // A special case for SRAI.
            if (funct3 == 0b101 && srai_bit == 1) {
                aluop_imm = AluOp::RightShiftA;
            }
            
            return aluop_imm;
        }
        // OP instructions have a variety of different ALU operations based on the
        // funct3 (and possibly funct7) value(s).
        case 0b0110011: {
            // ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND
            std::array<std::optional<AluOp>, 8> aluop_table = {
                AluOp::Add, AluOp::LeftShift, AluOp::Sub, AluOp::Sub,
                AluOp::Xor, AluOp::RightShiftL, AluOp::Or, AluOp::And
            };
            std::optional<AluOp> aluop = aluop_table[funct3];

            // If the m flag is turned on and funct7 is 1, then we can use multiply
            // and division ALU operations.
            if (m && funct7 == 0x1) {
                switch(funct3) {
                    case 0x0: // MUL
                        aluop = AluOp::Mul;
                        break;
                    case 0x1: // MULH
                        aluop = AluOp::Mul;
                        break;
                    case 0x2: // MULHSU
                        aluop = AluOp::Mul;
                        break;
                    case 0x3: // MULHU
                        aluop = AluOp::Mul;
                        break;
                    case 0x4: // DIV
                        aluop = AluOp::Div;
                        break;
                    case 0x5: // DIVU
                        aluop = AluOp::DivU;
                        break;
                    case 0x6: // REM
                        aluop = AluOp::Rem;
                        break;
                    case 0x7: // REMU
                        aluop = AluOp::RemU;
                        break;
                }
            } else if (!m && funct7 == 0x1) {
                return std::nullopt;
            }

            // SUB and SRA instructions can also be utilized.
            if (funct3 == 0b000 && funct7 == 0x20) {
                aluop = AluOp::Sub;
            } else if (funct3 == 0b101 && funct7 == 0x20) {
                aluop = AluOp::RightShiftA;
            }

            return aluop;
        }
        // SYSTEM instructions only have consideration for SFENCE.VMA at the moment.
        case 0b1110011: {
            if (funct3 == 0 && rd == 0 && funct7 == 0b0001001) {                // SFENCE.VMA
                return AluOp::Add;
            } else if (funct3 == 0 && rd == 0 && rs1 == 0 && immediate == 0) {  // ECALL
                return AluOp::Add;
            } else if (funct3 != 0b100 && funct3 != 0b000) {                    // CSRs
                return AluOp::Add;
            } else {
                return std::nullopt;
            }
        }
        // AUIPC instructions use ADD.
        case 0b0010111:
            return AluOp::Add;
        // LUI insrtuctions use ADD.
        case 0b0110111:
            return AluOp::Add;
        // AMO instructions are ADD for the memory address. Only if the a flag is turned on.
        case 0b0101111: {
            if (a && funct3 == 0x2) {
                switch ((funct7 & 0x7C) >> 2) {
                    case 0b00010: // LR.W
                        return AluOp::Add;
                    case 0b00011: // SC.W
                        return AluOp::Add;
                    case 0b00001: // AMOSWAP.W
                        return AluOp::Add;
                    case 0b00000: // AMOADD.W
                        return AluOp::Add;
                    case 0b00100: // AMOXOR.W
                        return AluOp::Add;
                    case 0b01100: // AMOAND.W
                        return AluOp::Add;
                    case 0b01000: // AMOOR.W
                        return AluOp::Add;
                    case 0b10000: // AMOMIN.W
                        return AluOp::Add;
                    case 0b10100: // AMOMAX.W
                        return AluOp::Add;
                    case 0b11000: // AMOMINU.W
                        return AluOp::Add;
                    case 0b11100: // AMOMAXU.W
                        return AluOp::Add;
                    default:
                        return std::nullopt;
                }
            } else {
                return std::nullopt;
            }
        }
        default:
            return std::nullopt;
    }
}

// Breaks down the fetched instruction and builds the Instruction struct.
Latch decode(Hart& hart, Latch fetched, bool m, bool a) {
    // If the instruction happens to be EBREAK, set it to a NOP.
    if (fetched.i.inst == 0x00100073) {
        fetched.i.inst = 0x00000013;
    }

    uint8_t opcode = fetched.i.inst & 0x7F;
    
    // Determines the type of instruction. If illegal, the per-HART latch is flagged as so.
    InstType type = classify_opcode(opcode);
    if (type == InstType::Illegal) {
        if (fetched.fault == ParkReason::Running)   fetched.fault = ParkReason::IllegalInstruction;
        return fetched;
    }

    // Extracts all field members based on instruction type. Fills in Instruction.rd field.
    auto [immediate, rd, funct3, rs1, rs2, funct7] = field_extraction(fetched.i.inst, type);
    fetched.i.rd = rd;

    // Sets the register sources for the instruction. If it is a particular Csr instruction
    // (CSRRW, CSRRS, CSRRC), only one register source is saved.
    if (opcode == 0b1110011 && (funct3 != 0 && funct3 != 0b100)) {
        if (funct3 <= 0b011)    fetched.rs1 = rs1;
    } else {
        fetched.rs1 = rs1;
        fetched.rs2 = rs2;
    }

    // Determines the ALU operation associated with the instruction. If it's illegal, the per-HART
    // latch is flagged as so and decode stops. Otherwise, the Instruction.aluop field is filled.
    std::optional<AluOp> aluop = aluop_decode(opcode, funct3, funct7, rd, rs1, (fetched.i.inst & 0x40000000) >> 30, immediate, m, a);
    if (aluop == std::nullopt) {
        if (fetched.fault == ParkReason::Running)   fetched.fault = ParkReason::IllegalInstruction;
        return fetched;
    }
    fetched.i.aluop = static_cast<uint8_t>(*aluop);

    // Fills the rest of the Instruction struct fields based on instruction type.
    switch (type) {
        // For R-type instructions, both operands come from the register file. If it is
        // an atomic instruction, store the value to combine with for later.
        case InstType::R:
            if ((opcode) == 0b0101111) {
                fetched.i.left = hart.read_register(rs1);
                fetched.i.disp = hart.read_register(rs2);

                if (((funct7 & 0x7C) >> 2) == 0b00010)  fetched.i.memop = 0b011;
                else                                    fetched.i.memop = 0b111;
            } else {
                fetched.i.left = hart.read_register(rs1);
                fetched.i.right = hart.read_register(rs2);
            }
            break;
        // For I-type instructions, the left operand comes from the register file, and
        // the right operand comes from the 12-bit immediate encoded in the instruction.
        case InstType::I:
            // In the event the instruction is a CSR immediate instruction, the left operand is
            // the unsigned immediate located in the rs1 field.
            if ((opcode) == 0b1110011 && (funct3 == 0b101 || funct3 == 0b110 || funct3 == 0b111)) {
                fetched.i.left = rs1;
            } else {
                fetched.i.left = hart.read_register(rs1);
            }

            // If the ALU operation is either SLL, SRL, or SRA, the shamt value is the right
            // operand. If the opcode matches the SYSTEM opcode, two things can occur: if the
            // funct3 value is 0, store another value from the register file; otherwise, it's
            // a CSR instruction and the right operand should be 0.
            if (aluop == AluOp::LeftShift || aluop == AluOp::RightShiftL || aluop == AluOp::RightShiftA) {
                fetched.i.right = (fetched.i.inst & 0x1F00000) >> 20;
            } else if ((opcode) == 0b1110011 && funct3 == 0b000) {
                fetched.i.disp = hart.read_register(rs2);
            } else if ((opcode) == 0b1110011 && funct3 != 0b000) {
                fetched.i.right = 0;
            } else {
                fetched.i.right = immediate;
            }

            // Determines the memop value based on the type of LOAD instruction.
            if ((opcode) == 0b0000011 && (funct3 == 0b000 || funct3 == 0b100))       fetched.i.memop = 0b001;
            else if ((opcode) == 0b0000011 && (funct3 == 0b001 || funct3 == 0b101))  fetched.i.memop = 0b010;
            else if ((opcode) == 0b0000011 && funct3 == 0b010)                       fetched.i.memop = 0b011;

            break;
        // The S-type instruction needs to use a register value plus an offset, and the
        // value of that needs to be stored comes from the register file.
        case InstType::S:
            fetched.i.left = hart.read_register(rs1);
            fetched.i.right = immediate;
            fetched.i.disp = hart.read_register(rs2);

            if (funct3 == 0b000)        fetched.i.memop = 0b101;
            else if (funct3 == 0b001)   fetched.i.memop = 0b110;
            else if (funct3 == 0b010)   fetched.i.memop = 0b111;

            break;
        // The B-type instruction acts like SLT, but the result decides what's added to the PC.
        case InstType::B:
            fetched.i.left = hart.read_register(rs1);
            fetched.i.right = hart.read_register(rs2);
            fetched.i.disp = immediate;
            break;
        // The AUIPC instruction adds the upper immediate to the PC of the AUIPC itself.
        // The LUI instruction adds the upper immediate to the value 0.
        case InstType::U:
            if ((opcode) == 0b0010111) {
                fetched.i.left = fetched.pc;
                fetched.i.right = immediate;
            } else if ((opcode) == 0b0110111) {
                fetched.i.left = 0;
                fetched.i.right = immediate;
            }
            break;
        // The J-type instruction adds the sign-extended 21-bit immediate to the PC.
        case InstType::J:
            fetched.i.disp = immediate;
            break;
        default:
            break;
    }

    return fetched;
}

// Rising edge for the decode stage of the instruction pipeline.
void decode_rising_edge(Hart& hart, HartPipelineObject& hpo, bool m, bool a) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // Resets the instruction status to ready every tick
    hpo.no_stall = true;

    // Decodes current fetched instruction and saves it for execute stage.
    hpo.id_ex_next = decode(hart, hpo.if_id_current, m, a);

    // Obtains the consumer's opcode, the producer's opcode, and whether or not the
    // producer's instruction's result is ready to forward when the consumer reaches execute.
    uint8_t opcode = hpo.id_ex_next.i.inst & 0x7F;
    uint8_t prod_id_ex_current_opcode = hpo.id_ex_current.i.inst & 0x7F;
    bool id_ex_current_val_ready = prod_id_ex_current_opcode != 0x2F && prod_id_ex_current_opcode != 0x73 && (hpo.id_ex_current.i.memop >= 0b100 || hpo.id_ex_current.i.memop == 0);

    // Bypasses into register source 1 (rs1) if the instruction in writeback's falling edge writes to the
    // register the consumer reads as rs1.
    if (hpo.id_ex_next.rs1 == hpo.mem_wb_current.i.rd && hpo.mem_wb_current.i.rd != 0) {
        hpo.id_ex_next.i.left = hpo.mem_wb_current.i.result;
    }

    // Bypasses into register source 2 (rs2) if the instruction in writeback's falling edge writes to the
    // register the consumer reads as rs2. It can belong in either the disp field for stores, AMO/SC and
    // SFENCE.VMA, or the right field for everything else.
    if (hpo.id_ex_next.rs2 == hpo.mem_wb_current.i.rd && hpo.mem_wb_current.i.rd != 0) {
        if (opcode == 0x23 || opcode == 0x2F || opcode == 0x73) hpo.id_ex_next.i.disp = hpo.mem_wb_current.i.result;
        else                                                    hpo.id_ex_next.i.right = hpo.mem_wb_current.i.result;
    }

    // Determines whether the consumer needs a value from the instruction one ahead that won't be ready in time.
    // If that's the case, a stall occurs.
    if (!id_ex_current_val_ready && (hpo.id_ex_current.i.rd == hpo.id_ex_next.rs1 || hpo.id_ex_current.i.rd == hpo.id_ex_next.rs2) && hpo.id_ex_current.i.rd != 0) {
        hpo.no_stall = false;
    }

    return;
}

// Falling edge for the decode stage of the instruction pipeline.
void decode_falling_edge(Hart& hart, HartPipelineObject& hpo) {
    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    // If a branch redirect is indicated, a NOP bubble is set up for execute. If a memory stall
    // occurs, hold the instruction. If a load-use stall occurs, bubble with NOP as well.
    // If no stalls occur, we set the decoded instruction up for execute stage.
    if (hpo.redirect) {
        hpo.id_ex_current = Latch();
        hpo.id_ex_current.i.inst = 0x00000013;
    } else if (!hpo.mem_stall && hpo.no_stall) {
        hpo.id_ex_current = hpo.id_ex_next;
    } else if (!hpo.mem_stall && !hpo.no_stall) {
        hpo.id_ex_current = Latch();
        hpo.id_ex_current.i.inst = 0x00000013;
    }

    return;
}