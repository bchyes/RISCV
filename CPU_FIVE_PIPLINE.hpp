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
        };

        bool IF_flag = 0;
        bool Branch_flag = 0;
        //bool busy[REG_SIZE];//仅仅用于load函数需要停顿的情况
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
    public:
        void RunFiveStagePipeline() {
            for (cycle = 0; cycle <= 45; cycle++) {
                IF();
                ID();
                EX();
                MEM();
                WB();
            }
            for (cycle = 46;; cycle++) {
                if (pc == 0x1000) {
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
        }

        void IF() {
            if (!IF_flag) {
                uint32_t command = GetCommand(pc);
                if (command == 0x0ff00513) {
                    IF_flag = 1;
                    return;
                }
                q_for_command.push({command, cycle, pc});
            }
        }

        void ID() {
            if (!q_for_command.empty()) {
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
                //bool stall = 0;
                if (q_for_mem.empty() && !wait_time) {
                    switch (code.type) {
                        case LUI:
                            ex.reg_pos = code.div.rd;
                            ex.val = code.imm;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case AUIPC:
                            ex.reg_pos = code.div.rd;
                            ex.val = code.imm + code.pc - 4;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case JAL:
                            ex.mem_pos = code.pc + code.imm - 4;
                            ex.jump = 1;
                            ex.reg_pos = code.div.rd;
                            ex.val = code.pc;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case JALR:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case BEQ:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BNE:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BLT:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BGE:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BLTU:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case BGEU:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case LB:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = 8;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case LH:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = 16;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case LW:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = 32;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case LBU:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = -8;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case LHU:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.mem_pos = forwarding_mem + code.imm;
                            } else {
                                ex.mem_pos = reg[code.div.rs1] + code.imm;
                            }
                            ex.load_bit = -16;
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = 0;
                            forwarding_ex = 0;
                            break;
                        case SB:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            //ex.reg_pos = code.div.rs2;
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
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            //ex.reg_pos = code.div.rs2;
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
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            //ex.reg_pos = code.div.rs2;
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
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex + code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem + code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] + code.imm;
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLTI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLTIU:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case XORI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex ^ code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem ^ code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] ^ code.imm;
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case ORI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex | code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem | code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] | code.imm;
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case ANDI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex & code.imm;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem & code.imm;
                            } else {
                                ex.val = reg[code.div.rs1] & code.imm;
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLLI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex << code.div.rs2;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem << code.div.rs2;
                            } else {
                                ex.val = reg[code.div.rs1] << code.div.rs2;
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRLI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = forwarding_ex >> code.div.rs2;
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = forwarding_mem >> code.div.rs2;
                            } else {
                                ex.val = reg[code.div.rs1] >> code.div.rs2;
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRAI:
                            /*if (busy[code.div.rs1]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
                            ex.reg_pos = code.div.rd;
                            if (forwarding_ex_reg == code.div.rs1) {
                                ex.val = ((int32_t) forwarding_ex) >> ((int32_t) code.div.rs2);
                            } else if (forwarding_mem_reg == code.div.rs1) {
                                ex.val = ((int32_t) forwarding_mem) >> ((int32_t) code.div.rs2);
                            } else {
                                ex.val = ((int32_t) reg[code.div.rs1]) >> ((int32_t) code.div.rs2);
                            }
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case ADD:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SUB:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLL:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLT:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SLTU:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case XOR:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRL:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case SRA:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case OR:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                        case AND:
                            /*if (busy[code.div.rs1] || busy[code.div.rs2]) {
                                stall = 1;
                                forwarding_ex_reg = 0;
                                forwarding_ex = 0;
                                break;
                            }*/
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
                            //busy[ex.reg_pos] = 1;
                            forwarding_ex_reg = ex.reg_pos;
                            forwarding_ex = ex.val;
                            break;
                    }
                    //if (!stall) {
                    q_for_code.pop();
                    ex.cycle = cycle;
                    q_for_mem.push(ex);
                    //}
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
                        //busy[wait_for_sl.reg_pos] = 0;
                        forwarding_mem_reg = wait_for_sl.reg_pos;
                        forwarding_mem = wait_for_sl.val;
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
                    if (!q_for_reg.empty()) return;
                    pc = ex.mem_pos;
                    if (ex.reg_pos && !Branch_flag) {
                        q_for_reg.push(ex);
                        reg[ex.reg_pos] = ex.val;
                        Branch_flag = 1;
                    }
                    IF_flag = 0;
                    while (!q_for_command.empty()) q_for_command.pop();
                    while (!q_for_code.empty()) q_for_code.pop();
                    while (!q_for_mem.empty()) q_for_mem.pop();
                    wait_time = 0;
                    /*for (int i = 0; i < 32; i++)
                        busy[i] = 0;*/
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
                //busy[ex.reg_pos] = 0;
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
