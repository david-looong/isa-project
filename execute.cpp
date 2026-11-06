/* Name: David Long
 * Date: November 4, 2026
 * Synopsis: This CPP file contains the contents of the previously declared
 * functions in execute.hpp.
 */

#include "execute.hpp"

// Computes the result of an arithmetic/logic operation between two operands as well as
// four status flags: NZCV for negative, zero, carry, and overflow.
AluOutput alu(uint32_t left, uint32_t right, uint8_t aluop) {
    AluOutput output;
    uint64_t result = 0;
    bool v_not = false;
    bool add_or_sub = false;

    if (aluop == static_cast<uint8_t>(AluOp::Add)) { // ADD
        result = static_cast<uint64_t>(left) + static_cast<uint64_t>(right);
        v_not = true;
        add_or_sub = true;
    } else if (aluop == static_cast<uint8_t>(AluOp::Sub)) { // SUB
        result = static_cast<uint64_t>(left) + static_cast<uint64_t>(~right) + 1;
        add_or_sub = true;
    } else if (aluop == static_cast<uint8_t>(AluOp::Mul)) { // MULTIPLY
        result = static_cast<uint64_t>(left) * static_cast<uint64_t>(right);
    } else if (aluop == static_cast<uint8_t>(AluOp::Div)) { // DIVIDE
        if (right == 0)                                         result = 0xFFFFFFFF;
        else if (left == 0x80000000 && right == 0xFFFFFFFF)     result = 0x80000000;
        else                                                    result = static_cast<uint32_t>(static_cast<int32_t>(left) / static_cast<int32_t>(right));
    } else if (aluop == static_cast<uint8_t>(AluOp::DivU)) { // DIVIDE UNSIGNED
        if (right == 0)                             result = 0xFFFFFFFF;
        else                                        result = left / right;
    } else if (aluop == static_cast<uint8_t>(AluOp::Rem)) { // REMAINDER
        if (right == 0)                                         result = left;
        else if (left == 0x80000000 && right == 0xFFFFFFFF)     result = 0;
        else                                                    result = static_cast<uint32_t>(static_cast<int32_t>(left) % static_cast<int32_t>(right));
    } else if (aluop == static_cast<uint8_t>(AluOp::RemU)) { // REMAINDER UNSIGNED
        if (right == 0)                             result = left;
        else                                        result = left % right;
    } else if (aluop == static_cast<uint8_t>(AluOp::LeftShift)) { // LOGICAL LEFT SHIFT
        result = left << (right & 0x1F);
    } else if (aluop == static_cast<uint8_t>(AluOp::RightShiftA)) { // ARITHMETIC RIGHT SHIFT
        result = static_cast<uint32_t>(static_cast<int32_t>(left) >> (right & 0x1F));
    } else if (aluop == static_cast<uint8_t>(AluOp::RightShiftL)) { // LOGICAL RIGHT SHIFT
        result = left >> (right & 0x1F);
    } else if (aluop == static_cast<uint8_t>(AluOp::Or)) { // OR
        result = left | right;
    } else if (aluop == static_cast<uint8_t>(AluOp::Xor)) { // XOR
        result = left ^ right;
    } else if (aluop == static_cast<uint8_t>(AluOp::And)) { // AND
        result = left & right;
    }

    // Checks to determine if the result is negative, zero or carries/borrows out of bit 31.
    if ((result >> 31) & 1)                 output.N = true;
    if ((result & 0xFFFFFFFF) == 0)         output.Z = true;
    if (add_or_sub && (result >> 32) & 1)   output.C = true;

    // Checks for signed overflow of the result.
    if (add_or_sub && v_not && (((~(left ^ right) & (left ^ result)) >> 31) & 1))      output.V = true;
    else if (add_or_sub && !v_not && ((((left ^ right) & (left ^ result)) >> 31) & 1)) output.V = true;

    // Returns the result.
    output.result = static_cast<uint32_t>(result);
    return output;
}

// Rising edge for the execute stage of the instruction pipeline.
void execute_rising_edge(Hart& hart, HartPipelineObject& hpo) {
    // Resets branch redirect/resolution status every tick.
    hpo.redirect = false;
    hpo.branch_resolved_tick = false;

    // If the instruction has a fault, the HART is parked and execute stops.
    if (hpo.id_ex_current.fault != ParkReason::Running) {
        hart.park(hpo.id_ex_current.fault);
        return;
    }

    // Does nothing if HART is parked.
    if (hart.is_parked()) {
        return;
    }

    hpo.ex_mem_next = hpo.id_ex_current;

    // Obtains the consumer's opcode, the producer's opcode, and whether or not the
    // producer's instruction's result is ready to forward when the consumer reaches memory.
    uint8_t opcode = hpo.ex_mem_next.i.inst & 0x7F;
    uint8_t prod_ex_mem_opcode = hpo.ex_mem_current.i.inst & 0x7F;
    bool ex_mem_val_ready = prod_ex_mem_opcode != 0x2F && prod_ex_mem_opcode != 0x73 && (hpo.ex_mem_current.i.memop >= 0b100 || hpo.ex_mem_current.i.memop == 0);

    // Bypasses into register source 1 (rs1) if the instruction in memory's falling edge writes to the 
    // register the consumer reads as rs1 and there isn't a stall. Otherwise, the bypass checks
    // writeback's falling edge for the same behavior.
    if (hpo.ex_mem_next.rs1 == hpo.ex_mem_current.i.rd && hpo.ex_mem_current.i.rd != 0) {
        if (ex_mem_val_ready)   hpo.ex_mem_next.i.left = hpo.ex_mem_current.i.result;
    } else if (hpo.ex_mem_next.rs1 == hpo.mem_wb_current.i.rd && hpo.mem_wb_current.i.rd != 0) {
        hpo.ex_mem_next.i.left = hpo.mem_wb_current.i.result;
    }

    // Bypasses into register source 2 (rs2) if the instruction in memory's falling edge writes to the 
    // register the consumer reads as rs1 and there isn't a stall. Otherwise, the bypass checks
    // writeback's falling edge for the same behavior. It can belong in either the disp field for stores, AMO/SC
    // and SFENCE.VMA, or the right field for everything else.
    if (hpo.ex_mem_next.rs2 == hpo.ex_mem_current.i.rd && hpo.ex_mem_current.i.rd != 0) {
        if (ex_mem_val_ready) {
            if (opcode == 0x23 || opcode == 0x2F || opcode == 0x73) hpo.ex_mem_next.i.disp = hpo.ex_mem_current.i.result;
            else                                                    hpo.ex_mem_next.i.right = hpo.ex_mem_current.i.result;
        }
    } else if (hpo.ex_mem_next.rs2 == hpo.mem_wb_current.i.rd && hpo.mem_wb_current.i.rd != 0) {
        if (opcode == 0x23 || opcode == 0x2F || opcode == 0x73) hpo.ex_mem_next.i.disp = hpo.mem_wb_current.i.result;
        else                                                    hpo.ex_mem_next.i.right = hpo.mem_wb_current.i.result;
    }

    // Executes current fetched instruction and saves it for execute stage.
    AluOutput output = alu(hpo.ex_mem_next.i.left, hpo.ex_mem_next.i.right, hpo.ex_mem_next.i.aluop);

    uint8_t funct3 = (hpo.ex_mem_next.i.inst & 0x7000) >> 12;
    uint32_t actual_next_pc;

    // Completes instruction-specific work the ALU cannot utilizing aluop, opcode, and funct3.
    if (hpo.ex_mem_next.i.aluop == static_cast<uint8_t>(AluOp::Sub)) {
        // A conditional branch has been resolved, so a predictor update is necessary.
        if (opcode == 0b1100011) {
            hpo.branch_resolved_tick = true;
            hpo.recorded_branch_pc = hpo.ex_mem_next.pc;

            // If the instruction is BEQ, BNE, BLT, BGE, BLTU, or BGEU (determined by funct3 and status flags),
            // the branch outcome is 1 (taken) and the next PC is the target. Otherwise, the outcome is 0 (not-taken)
            // and the next PC is its own PC + 4.
            if ((funct3 == 0x0 && output.Z == 1) || (funct3 == 0x1 && output.Z == 0)
            || (funct3 == 0x4 && output.N != output.V) || (funct3 == 0x5 && output.N == output.V)
            || (funct3 == 0x6 && output.C == 0) || (funct3 == 0x7 && output.C == 1)) {
                hpo.recorded_branch_outcome = 1;
                actual_next_pc = hpo.ex_mem_next.pc + hpo.ex_mem_next.i.disp;
            } else {
                hpo.recorded_branch_outcome = 0;
                actual_next_pc = hpo.ex_mem_next.pc + 4;
            }

            // If the actual next PC disagrees with fetch's guess, it's a misprediction and needs a redirect
            // to the correct PC.
            if (actual_next_pc != hpo.ex_mem_next.predicted_next_pc) {
                hpo.redirect = true;
                hpo.redirect_pc = actual_next_pc;
            }
        // SLT[U] and SLTI[U] replace the subtraction result with 0 or 1 using status flag conditions.
        } else if ((opcode == 0b0110011 || opcode == 0b0010011) && funct3 == 0x2) {
            output.result = (output.N != output.V) ? 1 : 0;
        } else if ((opcode == 0b0110011 || opcode == 0b0010011) && funct3 == 0x3) {
            output.result = (output.C == 0) ? 1 : 0;
        }
    // Specifically for JAL and JALR instructions. All other ADDS are ignored.
    } else if (hpo.ex_mem_next.i.aluop == static_cast<uint8_t>(AluOp::Add)) {
        if (opcode == 0b1101111 || opcode == 0b1100111) {
            // Next PC is always the target since jumps are always taken
            // JAL: own PC + disp | JALR: ALU's rs1 + imm with bit 0 cleared.
            actual_next_pc = (opcode == 0b1101111) ? hpo.ex_mem_next.pc + hpo.ex_mem_next.i.disp : output.result & ~1;
            output.result = hpo.ex_mem_next.pc + 4;

            // If the actual next PC disagrees with fetch's guess, it's a misprediction and needs a redirect
            // to the correct PC.
            if (actual_next_pc != hpo.ex_mem_next.predicted_next_pc) {
                hpo.redirect = true;
                hpo.redirect_pc = actual_next_pc;
            }
        }
    // Specifically for MULH, MULHSU, and MULHU since the ALU cannot tell those apart.
    } else if (hpo.ex_mem_next.i.aluop == static_cast<uint8_t>(AluOp::Mul)) {
        if (funct3 == 0b001) { // MULH
            output.result = (static_cast<int64_t>(static_cast<int32_t>(hpo.ex_mem_next.i.left)) * static_cast<int64_t>(static_cast<int32_t>(hpo.ex_mem_next.i.right))) >> 32;
        } else if (funct3 == 0b010) { // MULHSU
            output.result = (static_cast<int64_t>(static_cast<int32_t>(hpo.ex_mem_next.i.left)) * static_cast<int64_t>(hpo.ex_mem_next.i.right)) >> 32;
        } else if (funct3 == 0b011) { // MULHU
            output.result = (static_cast<uint64_t>(hpo.ex_mem_next.i.left) * static_cast<uint64_t>(hpo.ex_mem_next.i.right)) >> 32;
        }
    }

    // Assign the result to the appropriate latch.
    hpo.ex_mem_next.i.result = output.result;
    return;
}

// Falling edge for the execute stage of the instruction pipeline.
void execute_falling_edge(Hart& hart, HartPipelineObject& hpo) {
    // Sets instruction as a NOP if the HART is parked.
    if (hart.is_parked()) {
        hpo.ex_mem_current = Latch();
        hpo.ex_mem_current.i.inst = 0x00000013;
        return;
    }

    // If branch resolution is indicated, update the branch predictor table with the new branch outcome.
    if (hpo.branch_resolved_tick) {
        hart.update_bp(hpo.recorded_branch_pc, hpo.recorded_branch_outcome);
    }

    // Sets executed instruction up for memory stage.
    hpo.ex_mem_current = hpo.ex_mem_next;
    return;
}