//
// Created by 25693 on 2022/6/23.
//

#ifndef RISCV_CPU_FIVE_PIPLINE_HPP
#define RISCV_CPU_FIVE_PIPLINE_HPP

#include "CPU.hpp"

namespace RISCV {
    class CPU_FIVE_PIPELINE : public CPU {
    private:
        enum CommandType {
            LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU, LHU,
            SB, SH, SW, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD, SUB,
            SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
        };
        enum BranchType {
            NO, BEQ_branch, BNE_branch, BLT_branch, BGE_branch, BLTU_branch, BGEU_branch
        };
        uint32_t cycle = 0;
        struct Command {
            uint32_t command;
            uint32_t cycle;
            uint32_t pc;
        };

        struct Decode {
            Divide div;
            int32_t imm = 0;
            CommandType type;
            uint32_t cycle;
            uint32_t pc;

            explicit Decode() = default;
        };

        struct Execute {
            uint8_t reg_pos = 0;
            uint32_t mem_pos = 0;//记录跳转或者读写的位置
            uint32_t val = 0;
            int8_t load_bit = 0;//约定符号为无符号扩展
            int8_t save_bit = 0;
            bool jump = 0;
            uint32_t cycle;
            BranchType branch = NO;
        };

        bool IF_flag = 0;
        bool Branch_flag = 0;
        bool wait_for_load = 0;
        std::queue<Command> q_for_command;
        std::queue<Decode> q_for_code;
        std::queue<Execute> q_for_mem;
        std::queue<Execute> q_for_reg;
        uint8_t wait_time = 0;
        Execute wait_for_sl;
        uint8_t forwarding_mem_reg;
        uint32_t forwarding_mem;
        uint8_t forwarding_ex_reg;
        uint32_t forwarding_ex;
        int32_t tmp_reg_rs1;
        int32_t tmp_reg_rs2;
        uint32_t tmp_reg_rs1_u;
        uint32_t tmp_reg_rs2_u;
        uint8_t Beq_branch = 0;
        uint8_t Bne_branch = 0;
        uint8_t Blt_branch = 0;
        uint8_t Bge_branch = 0;
        uint8_t Bltu_branch = 0;
        uint8_t Bgeu_branch = 0;
        uint32_t Beq_branch_pc = 0;
        uint32_t Bne_branch_pc = 0;
        uint32_t Blt_branch_pc = 0;
        uint32_t Bge_branch_pc = 0;
        uint32_t Bltu_branch_pc = 0;
        uint32_t Bgeu_branch_pc = 0;
    public:
        void RunFiveStagePipeline() {
            for (cycle = 0;; cycle++) {
                if (cycle == 278) {
                    int x = 1;
                }
                IF();
                ID();
                EX();
                MEM();
                WB();
                if (IF_flag && q_for_command.empty() && q_for_code.empty() && q_for_mem.empty() && q_for_reg.empty()) {
                    printf("%u\n", GetBitBetween(reg[10], 0, 7));
                    break;
                }
            }
            std::cerr << cycle;
        }

        void IF() {
            if (!IF_flag && q_for_command.size() <= 1) {
                uint32_t command = GetCommand(pc);
                if (command == 0x0ff00513) {
                    IF_flag = 1;
                    return;
                }
                if (command % (1 << 7) == 0b1100011) {
                    switch ((command >> 12) % (1 << 3)) {
                        case 0b0:
                            if (Beq_branch == 2 || Beq_branch == 3) {
                                int32_t imm = GetimmB(Divide(command));
                                Beq_branch_pc = pc;
                                pc += imm - 4;
                            }
                            break;
                        case 0b1:
                            if (Bne_branch == 2 || Bne_branch == 3) {
                                int32_t imm = GetimmB(Divide(command));
                                Bne_branch_pc = pc;
                                pc += imm - 4;
                            }
                            break;
                        case 0b100:
                            if (Blt_branch == 2 || Blt_branch == 3) {
                                int32_t imm = GetimmB(Divide(command));
                                Blt_branch_pc = pc;
                                pc += imm - 4;
                            }
                            break;
                        case 0b101:
                            if (Bge_branch == 2 || Bge_branch == 3) {
                                int32_t imm = GetimmB(Divide(command));
                                Bge_branch_pc = pc;
                                pc += imm - 4;
                            }
                            break;
                        case 0b110:
                            if (Bltu_branch == 2 || Bltu_branch == 3) {
                                int32_t imm = GetimmB(Divide(command));
                                Bltu_branch_pc = pc;
                                pc += imm - 4;
                            }
                            break;
                        case 0b111:
                            if (Bgeu_branch == 2 || Bgeu_branch == 3) {
                                int32_t imm = GetimmB(Divide(command));
                                Bgeu_branch_pc = pc;
                                pc += imm - 4;
                            }
                            break;
                    }
                }/* else if (command % (1 << 7) == 0b1100111) {
                    int32_t imm = GetimmI(Divide(command));

                } else if (command % (1 << 7) == 0b1101111) {
                    int32_t imm = GetimmJ(Divide(command));
                }*/
                q_for_command.push({command, cycle, pc});
            }
        }

        void ID() {
            if (!q_for_command.empty() && q_for_code.size() <= 1) {
                Command command = q_for_command.front();
                if (cycle == command.cycle) return;
                q_for_command.pop();
                Divide div = GetDivide(command.command);
                Decode code;
                code.div = div;
                switch (div.opcode) {
                    case 0b110111:
                        U_type(code);
                        break;//LUI
                    case 0b10111:
                        U_type(code);
                        break;//AUIPC
                    case 0b1101111:
                        J_type(code);
                        break;//JAL
                    case 0b1100111:
                        I_type(code);
                        break;//JALR
                    case 0b1100011:
                        B_type(code);
                        break;
                    case 0b11:
                        I_type(code);
                        break;
                    case 0b100011:
                        S_type(code);
                        break;
                    case 0b10011:
                        I_type(code);
                        break;
                    case 0b110011:
                        R_type(code);
                        break;
                }
                code.cycle = cycle;
                code.pc = command.pc;
                q_for_code.push(code);
            }
        }

        void EX() {
            if (!q_for_code.empty()) {
                Decode code = q_for_code.front();
                if (code.cycle == cycle) return;
                Execute ex;
                if (q_for_mem.size() <= 1 && !wait_time && !wait_for_load) {
                    switch (code.type) {
                        case LUI:
                            ex.reg_pos = code.div.rd;
                            ex.val = code.imm;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case AUIPC:
                            ex.reg_pos = code.div.rd;
                            ex.val = code.imm + code.pc - 4;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case JAL:
                            ex.mem_pos = code.pc + code.imm - 4;
                            ex.jump = 1;
                            ex.reg_pos = code.div.rd;
                            ex.val = code.pc;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case JALR:
                            ex.val = code.pc;
                            ex.jump = 1;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = (forwarding_ex + code.imm) & (~1);
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = (forwarding_mem + code.imm) & (~1);
                            } else {
                                ex.mem_pos = (reg[code.div.rs1] + code.imm) & (~1);
                            }
                            ex.reg_pos = code.div.rd;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case BEQ:
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_mem;
                            } else {
                                tmp_reg_rs1_u = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_mem;
                            } else {
                                tmp_reg_rs2_u = reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1_u == tmp_reg_rs2_u) {
                                ex.mem_pos = code.pc + code.imm - 4;
                                ex.jump = 1;
                            }
                            ex.branch = BEQ_branch;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BNE:
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_mem;
                            } else {
                                tmp_reg_rs1_u = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_mem;
                            } else {
                                tmp_reg_rs2_u = reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1_u != tmp_reg_rs2_u) {
                                ex.mem_pos = code.pc + code.imm - 4;
                                ex.jump = 1;
                            }
                            ex.branch = BNE_branch;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BLT:
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1 = (int32_t) forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1 = (int32_t) forwarding_mem;
                            } else {
                                tmp_reg_rs1 = (int32_t) reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2 = (int32_t) forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2 = (int32_t) forwarding_mem;
                            } else {
                                tmp_reg_rs2 = (int32_t) reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1 < tmp_reg_rs2) {
                                ex.mem_pos = code.pc + code.imm - 4;
                                ex.jump = 1;
                            }
                            ex.branch = BLT_branch;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BGE:
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1 = (int32_t) forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1 = (int32_t) forwarding_mem;
                            } else {
                                tmp_reg_rs1 = (int32_t) reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2 = (int32_t) forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2 = (int32_t) forwarding_mem;
                            } else {
                                tmp_reg_rs2 = (int32_t) reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1 >= tmp_reg_rs2) {
                                ex.mem_pos = code.pc + code.imm - 4;
                                ex.jump = 1;
                            }
                            ex.branch = BGE_branch;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BLTU:
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_mem;
                            } else {
                                tmp_reg_rs1_u = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_mem;
                            } else {
                                tmp_reg_rs2_u = reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1_u < tmp_reg_rs2_u) {
                                ex.mem_pos = code.pc + code.imm - 4;
                                ex.jump = 1;
                            }
                            ex.branch = BLTU_branch;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BGEU:
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_mem;
                            } else {
                                tmp_reg_rs1_u = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_mem;
                            } else {
                                tmp_reg_rs2_u = reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1_u >= tmp_reg_rs2_u) {
                                ex.mem_pos = code.pc + code.imm - 4;
                                ex.jump = 1;
                            }
                            ex.branch = BGEU_branch;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case LB:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = 8;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            wait_for_load = 1;
                            break;
                        case LH:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = 16;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            wait_for_load = 1;
                            break;
                        case LW:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = 32;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            wait_for_load = 1;
                            break;
                        case LBU:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = -8;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            wait_for_load = 1;
                            break;
                        case LHU:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = -16;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            wait_for_load = 1;
                            break;
                        case SB:
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val = GetBitBetween(forwarding_ex, 0, 7);
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val = GetBitBetween(forwarding_mem, 0, 7);
                            } else {
                                ex.val = GetBitBetween(reg[code.div.rs2], 0, 7);
                            }
                            ex.save_bit = 8;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case SH:
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val = GetBitBetween(forwarding_ex, 0, 15);
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val = GetBitBetween(forwarding_mem, 0, 15);
                            } else {
                                ex.val = GetBitBetween(reg[code.div.rs2], 0, 15);
                            }
                            ex.save_bit = 16;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case SW:
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs2];
                            }
                            ex.save_bit = 32;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case ADDI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem + code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] + code.imm;
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLTI:
                            if (forwarding_ex_reg == code.div.rs1) {
                                if ((int32_t) forwarding_ex < code.imm) ex.val = 1;
                                else ex.val = 0;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                if ((int32_t) forwarding_mem < code.imm) ex.val = 1;
                                else ex.val = 0;
                            } else {
                                if ((int32_t) reg[code.div.rs1] < code.imm) ex.val = 1;
                                else ex.val = 0;
                            }
                            ex.reg_pos = code.div.rd;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLTIU:
                            if (forwarding_ex_reg == code.div.rs1) {
                                if (forwarding_ex < (uint32_t) code.imm) ex.val = 1;
                                else ex.val = 0;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                if (forwarding_mem < (uint32_t) code.imm) ex.val = 1;
                                else ex.val = 0;
                            } else {
                                if (reg[code.div.rs1] < (uint32_t) code.imm) ex.val = 1;
                                else ex.val = 0;
                            }
                            if (reg[code.div.rs1] < (uint32_t) code.imm) ex.val = 1;
                            else ex.val = 0;
                            ex.reg_pos = code.div.rd;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case XORI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex ^ code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem ^ code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] ^ code.imm;
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case ORI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex | code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem | code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] | code.imm;
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case ANDI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex & code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem & code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] & code.imm;
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLLI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex << code.div.rs2;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem << code.div.rs2;
                            } else {
                                ex.val = reg[code.div.rs1] << code.div.rs2;
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRLI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex >> code.div.rs2;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem >> code.div.rs2;
                            } else {
                                ex.val = reg[code.div.rs1] >> code.div.rs2;
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRAI:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = ((int32_t) forwarding_ex) >> ((int32_t) code.div.rs2);
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = ((int32_t) forwarding_mem) >> ((int32_t) code.div.rs2);
                            } else {
                                ex.val = ((int32_t) reg[code.div.rs1]) >> ((int32_t) code.div.rs2);
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case ADD:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val += forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val += forwarding_mem;
                            } else {
                                ex.val += reg[code.div.rs2];
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SUB:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val -= forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val -= forwarding_mem;
                            } else {
                                ex.val -= reg[code.div.rs2];
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLL:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val <<= GetBitBetween(forwarding_ex, 0, 4);
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val <<= GetBitBetween(forwarding_mem, 0, 4);
                            } else {
                                ex.val <<= GetBitBetween(reg[code.div.rs2], 0, 4);
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLT:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1 = (int32_t) forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1 = (int32_t) forwarding_mem;
                            } else {
                                tmp_reg_rs1 = (int32_t) reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2 = (int32_t) forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2 = (int32_t) forwarding_mem;
                            } else {
                                tmp_reg_rs2 = (int32_t) reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1 < tmp_reg_rs2) ex.val = 1; else ex.val = 0;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLTU:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                tmp_reg_rs1_u = forwarding_mem;
                            } else {
                                tmp_reg_rs1_u = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                tmp_reg_rs2_u = forwarding_mem;
                            } else {
                                tmp_reg_rs2_u = reg[code.div.rs2];
                            }
                            if (tmp_reg_rs1_u < tmp_reg_rs2_u) ex.val = 1; else ex.val = 0;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case XOR:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val ^= forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val ^= forwarding_mem;
                            } else {
                                ex.val ^= reg[code.div.rs2];
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRL:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val >>= GetBitBetween(forwarding_ex, 0, 4);
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val >>= GetBitBetween(forwarding_mem, 0, 4);
                            } else {
                                ex.val >>= GetBitBetween(reg[code.div.rs2], 0, 4);
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRA:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val >>= ((int32_t) GetBitBetween(forwarding_ex, 0, 4));
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val >>= ((int32_t) GetBitBetween(forwarding_mem, 0, 4));
                            } else {
                                ex.val >>= ((int32_t) GetBitBetween(reg[code.div.rs2], 0, 4));
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case OR:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val |= forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val |= forwarding_mem;
                            } else {
                                ex.val |= reg[code.div.rs2];
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case AND:
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem;
                            } else {
                                ex.val = reg[code.div.rs1];
                            }
                            if (forwarding_ex_reg == code.div.rs2) {
                                ex.val &= forwarding_ex;
                            } else if (forwarding_mem_reg == code.div.rs2) {
                                ex.val &= forwarding_mem;
                            } else {
                                ex.val &= reg[code.div.rs2];
                            }
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                    }
                    q_for_code.pop();
                    ex.cycle = cycle;
                    q_for_mem.push(ex);
                }
            }
        }

        void MEM() {
            if (wait_time) {
                wait_time++;
                if (wait_time == 3) {
                    if (wait_for_sl.load_bit) {
                        switch (wait_for_sl.load_bit) {
                            case 8:
                                wait_for_sl.val = signedExtend<8>(memory.getPos8(wait_for_sl.mem_pos));
                                break;
                            case 16:
                                wait_for_sl.val = signedExtend<16>(memory.getPos16(wait_for_sl.mem_pos));
                                break;
                            case 32:
                                wait_for_sl.val = memory.getPos32(wait_for_sl.mem_pos);
                                break;
                            case -8:
                                wait_for_sl.val = memory.getPos8(wait_for_sl.mem_pos);
                                break;
                            case -16:
                                wait_for_sl.val = memory.getPos16(wait_for_sl.mem_pos);
                                break;
                        }
                        wait_for_sl.cycle = cycle;
                        forwarding_mem_reg = wait_for_sl.reg_pos;
                        forwarding_mem = wait_for_sl.val;
                        wait_for_load = 0;
                        q_for_reg.push(wait_for_sl);
                    } else if (wait_for_sl.save_bit) {
                        switch (wait_for_sl.save_bit) {
                            case 8:
                                memory.getPos8(wait_for_sl.mem_pos) = wait_for_sl.val;
                                break;
                            case 16:
                                memory.getPos16(wait_for_sl.mem_pos) = wait_for_sl.val;
                                break;
                            case 32:
                                memory.getPos32(wait_for_sl.mem_pos) = wait_for_sl.val;
                                break;
                        }
                        forwarding_mem_reg = 0;
                        forwarding_mem = 0;
                    }
                    wait_time = 0;
                }
                return;
            }
            if (!q_for_mem.empty()) {
                Execute ex = q_for_mem.front();
                if (ex.cycle == cycle) return;
                if (ex.jump) {
                    if (ex.branch != NO) {
                        switch (ex.branch) {
                            case BEQ_branch:
                                if (Beq_branch == 2 || Beq_branch == 3) {
                                    Beq_branch = 3;
                                    q_for_mem.pop();
                                    return;
                                }
                                break;
                            case BNE_branch:
                                if (Bne_branch == 2 || Bne_branch == 3) {
                                    Bne_branch = 3;
                                    q_for_mem.pop();
                                    return;
                                }
                                break;
                            case BLT_branch:
                                if (Blt_branch == 2 || Blt_branch == 3) {
                                    Blt_branch = 3;
                                    q_for_mem.pop();
                                    return;
                                }
                                break;
                            case BGE_branch:
                                if (Bge_branch == 2 || Bge_branch == 3) {
                                    Bge_branch = 3;
                                    q_for_mem.pop();
                                    return;
                                }
                                break;
                            case BLTU_branch:
                                if (Bltu_branch == 2 || Bltu_branch == 3) {
                                    Bltu_branch = 3;
                                    q_for_mem.pop();
                                    return;
                                }
                                break;
                            case BGEU_branch:
                                if (Bgeu_branch == 2 || Bgeu_branch == 3) {
                                    Bgeu_branch = 3;
                                    q_for_mem.pop();
                                    return;
                                }
                                break;
                        }
                    }
                    if (!q_for_reg.empty()) return;
                    if (ex.reg_pos && !Branch_flag) {
                        ex.cycle = cycle;
                        q_for_reg.push(ex);
                        Branch_flag = 1;
                        return;
                    }
                    switch (ex.branch) {
                        case BEQ_branch:
                            if (Beq_branch == 0)
                                Beq_branch = 1;
                            else if (Beq_branch == 1)
                                Beq_branch = 2;
                            break;
                        case BNE_branch:
                            if (Bne_branch == 0)
                                Bne_branch = 1;
                            else if (Bne_branch == 1)
                                Bne_branch = 2;
                            break;
                        case BLT_branch:
                            if (Blt_branch == 0)
                                Blt_branch = 1;
                            else if (Blt_branch == 1)
                                Blt_branch = 2;
                            break;
                        case BGE_branch:
                            if (Bge_branch == 0)
                                Bge_branch = 1;
                            else if (Bge_branch == 1)
                                Bge_branch = 2;
                            break;
                        case BLTU_branch:
                            if (Bltu_branch == 0)
                                Bltu_branch = 1;
                            else if (Bltu_branch == 1)
                                Bltu_branch = 2;
                            break;
                        case BGEU_branch:
                            if (Bgeu_branch == 0)
                                Bgeu_branch = 1;
                            else if (Bgeu_branch == 1)
                                Bgeu_branch = 2;
                            break;
                    }
                    pc = ex.mem_pos;
                    IF_flag = 0;
                    wait_for_load = 0;
                    while (!q_for_command.empty()) q_for_command.pop();
                    while (!q_for_code.empty()) q_for_code.pop();
                    while (!q_for_mem.empty()) q_for_mem.pop();
                    wait_time = 0;
                    forwarding_ex = 0;
                    forwarding_ex_reg = 0;
                    forwarding_mem = 0;
                    forwarding_mem_reg = 0;
                    Branch_flag = 0;
                    return;
                } else if (ex.branch != NO) {
                    switch (ex.branch) {
                        case BEQ_branch:
                            if (Beq_branch == 0 || Beq_branch == 1) {
                                Beq_branch = 0;
                                q_for_mem.pop();
                                return;
                            }
                            break;
                        case BNE_branch:
                            if (Bne_branch == 0 || Bne_branch == 1) {
                                Bne_branch = 0;
                                q_for_mem.pop();
                                return;
                            }
                            break;
                        case BLT_branch:
                            if (Blt_branch == 0 || Blt_branch == 1) {
                                Blt_branch = 0;
                                q_for_mem.pop();
                                return;
                            }
                            break;
                        case BGE_branch:
                            if (Bge_branch == 0 || Bge_branch == 1) {
                                Bge_branch = 0;
                                q_for_mem.pop();
                                return;
                            }
                            break;
                        case BLTU_branch:
                            if (Bltu_branch == 0 || Bltu_branch == 1) {
                                Bltu_branch = 0;
                                q_for_mem.pop();
                                return;
                            }
                            break;
                        case BGEU_branch:
                            if (Bgeu_branch == 0 || Bgeu_branch == 1) {
                                Bgeu_branch = 0;
                                q_for_mem.pop();
                                return;
                            }
                            break;
                    }
                    if (!q_for_reg.empty()) return;
                    switch (ex.branch) {
                        case BEQ_branch:
                            if (Beq_branch == 2)
                                Beq_branch = 1;
                            else if (Beq_branch == 3)
                                Beq_branch = 2;
                            pc = Beq_branch_pc;
                            break;
                        case BNE_branch:
                            if (Bne_branch == 2)
                                Bne_branch = 1;
                            else if (Bne_branch == 3)
                                Bne_branch = 2;
                            pc = Bne_branch_pc;
                            break;
                        case BLT_branch:
                            if (Blt_branch == 2)
                                Blt_branch = 1;
                            else if (Blt_branch == 3)
                                Blt_branch = 2;
                            pc = Blt_branch_pc;
                            break;
                        case BGE_branch:
                            if (Bge_branch == 2)
                                Bge_branch = 1;
                            else if (Bge_branch == 3)
                                Bge_branch = 2;
                            pc = Bge_branch_pc;
                            break;
                        case BLTU_branch:
                            if (Bltu_branch == 2)
                                Bltu_branch = 1;
                            else if (Bltu_branch == 3)
                                Bltu_branch = 2;
                            pc = Bltu_branch_pc;
                            break;
                        case BGEU_branch:
                            if (Bgeu_branch == 2)
                                Bgeu_branch = 1;
                            else if (Bgeu_branch == 3)
                                Bgeu_branch = 2;
                            pc = Bgeu_branch_pc;
                            break;
                    }
                    IF_flag = 0;
                    wait_for_load = 0;
                    while (!q_for_command.empty()) q_for_command.pop();
                    while (!q_for_code.empty()) q_for_code.pop();
                    while (!q_for_mem.empty()) q_for_mem.pop();
                    wait_time = 0;
                    forwarding_ex = 0;
                    forwarding_ex_reg = 0;
                    forwarding_mem = 0;
                    forwarding_mem_reg = 0;
                    Branch_flag = 0;
                    return;
                } else if (ex.load_bit || ex.save_bit) {
                    wait_for_sl = ex;
                    wait_time++;
                    forwarding_mem_reg = 0;
                    forwarding_mem = 0;
                } else {
                    ex.cycle = cycle;
                    q_for_reg.push(ex);
                    forwarding_mem_reg = ex.reg_pos;
                    forwarding_mem = ex.val;
                }
                q_for_mem.pop();
            }
        }

        void WB() {
            if (!q_for_reg.empty()) {
                Execute ex = q_for_reg.front();
                if (ex.cycle == cycle) return;
                q_for_reg.pop();
                reg[ex.reg_pos] = ex.val;
            }
        }

        void U_type(Decode &code) {
            code.imm = GetimmU(code.div);
            switch (code.div.opcode) {
                case 0b110111:
                    code.type = LUI;
                    break;
                case 0b10111:
                    code.type = AUIPC;
                    break;
            }
        }

        void J_type(Decode &code) {
            code.imm = GetimmJ(code.div);
            code.type = JAL;
        }

        void I_type(Decode &code) {
            code.imm = GetimmI(code.div);
            if (code.div.opcode == 0b1100111) {
                code.type = JALR;
            } else if (code.div.opcode == 0b11) {
                switch (code.div.funct3) {
                    case 0b0:
                        code.type = LB;
                        break;
                    case 0b1:
                        code.type = LH;
                        break;
                    case 0b10:
                        code.type = LW;
                        break;
                    case 0b100:
                        code.type = LBU;
                        break;
                    case 0b101:
                        code.type = LHU;
                        break;
                }
            } else if (code.div.opcode == 0b10011) {
                switch (code.div.funct3) {
                    case 0b0:
                        code.type = ADDI;
                        break;
                    case 0b10:
                        code.type = SLTI;
                        break;
                    case 0b11:
                        code.type = SLTIU;
                        break;
                    case 0b100:
                        code.type = XORI;
                        break;
                    case 0b110:
                        code.type = ORI;
                        break;
                    case 0b111:
                        code.type = ANDI;
                        break;
                    case 0b001:
                        code.imm = code.div.rs2;
                        code.type = SLLI;
                        break;
                    default:
                        switch (code.div.funct7) {
                            case 0b0:
                                code.imm = code.div.rs2;
                                code.type = SRLI;
                                break;
                            default:
                                code.imm = code.div.rs2;
                                code.type = SRAI;
                                break;
                        }
                        break;
                }
            }
        }

        void B_type(Decode &code) {
            code.imm = GetimmB(code.div);
            switch (code.div.funct3) {
                case 0b0:
                    code.type = BEQ;
                    break;
                case 0b1:
                    code.type = BNE;
                    break;
                case 0b100:
                    code.type = BLT;
                    break;
                case 0b101:
                    code.type = BGE;
                    break;
                case 0b110:
                    code.type = BLTU;
                    break;
                case 0b111:
                    code.type = BGEU;
                    break;
            }
        }

        void S_type(Decode &code) {
            code.imm = GetimmS(code.div);
            switch (code.div.funct3) {
                case 0b0:
                    code.type = SB;
                    break;
                case 0b1:
                    code.type = SH;
                    break;
                case 0b10:
                    code.type = SW;
                    break;
            }
        }

        void R_type(Decode &code) {
            if (code.div.funct7 == 0b0) {
                switch (code.div.funct3) {
                    case 0b0:
                        code.type = ADD;
                        break;
                    case 0b1:
                        code.type = SLL;
                        break;
                    case 0b10:
                        code.type = SLT;
                        break;
                    case 0b11:
                        code.type = SLTU;
                        break;
                    case 0b100:
                        code.type = XOR;
                        break;
                    case 0b101:
                        code.type = SRL;
                        break;
                    case 0b110:
                        code.type = OR;
                        break;
                    case 0b111:
                        code.type = AND;
                        break;
                }
            } else {
                switch (code.div.funct3) {
                    case 0b0:
                        code.type = SUB;
                        break;
                    case 0b101:
                        code.type = SRA;
                        break;
                }
            }
        }
    };
}
#endif //RISCV_CPU_FIVE_PIPLINE_HPP
