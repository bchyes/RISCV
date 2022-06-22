//
// Created by 25693 on 2022/6/21.
//

#ifndef RISCV_CPU_HPP
#define RISCV_CPU_HPP

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <cstring>
#include "BitOperator.hpp"

namespace RISCV {
    class CPU {
    private:
        static const uint32_t MEM_SIZE = 600000;
        static const uint8_t REG_SIZE = 32;

        template<uint32_t SIZE = MEM_SIZE>
        struct Memory {
            uint8_t val[SIZE] = {0};

            uint8_t &operator[](uint32_t pos) {
                return val[pos];
            }

            uint8_t &getPos8(uint32_t pos) {
                return *((uint8_t *) (val + pos));
            }

            uint16_t &getPos16(uint32_t pos) {
                return *((uint16_t *) (val + pos));
            }

            uint32_t &getPos32(uint32_t pos) {
                return *((uint32_t *) (val + pos));
            }
        };

        Memory<> memory;

        template<uint8_t SIZE = REG_SIZE>
        struct Register {
            uint32_t val[SIZE] = {0};

            uint32_t &operator[](const uint32_t &pos) {
                return val[pos];
            }
        };

        Register<> reg;

        uint32_t pc = 0;
    public:
        void ReadMemory(FILE *input) {
            char str[20];
            uint32_t pos = 0;
            uint32_t value;
            while (fscanf(input, "%s", str) == 1) {
                if (str[0] == '@') {
                    sscanf(str, "@%x", &pos);
                } else {
                    sscanf(str, "%x", &value);
                    memory[pos] = (uint8_t) value;
                    pos++;
                }
            }
        }

        uint32_t GetCommand(uint32_t &pos) {
            uint32_t value = 0;
            for (int i = 1; i <= 4; i++) {
                value |= (uint32_t) (memory.val[pos + i - 1]) * (1 << (8 * i - 8));
            }
            pos += 4;
            return value;
        }

        void Run() {
            for (int cycle = 0;; cycle++) {
                uint32_t command = GetCommand(pc);
                Divide div = GetDivide(command);
                if (div.command == 0x0ff00513) {
                    printf("%u\n", GetBitBetween(reg[10], 0, 7));
                    break;
                }
                switch (div.opcode) {
                    case 0b110111:
                        U_type(div);
                        break;//LUI
                    case 0b10111:
                        U_type(div);
                        break;//AUIPC
                    case 0b1101111:
                        J_type(div);
                        break;//JAL
                    case 0b1100111:
                        I_type(div);
                        break;//JALR
                    case 0b1100011:
                        B_type(div);
                        break;
                    case 0b11:
                        I_type(div);
                        break;
                    case 0b100011:
                        S_type(div);
                        break;
                    case 0b10011:
                        I_type(div);
                        break;
                    case 0b110011:
                        R_type(div);
                        break;
                }
            }
        }

        void LUI(const Divide &div, const int32_t &imm) {
            reg[div.rd] = imm;
        }

        void AUIPC(const Divide &div, const int32_t &imm) {
            reg[div.rd] = pc + imm - 4;
        }

        void U_type(const Divide &div) {
            int32_t imm = GetimmU(div);
            switch (div.opcode) {
                case 0b110111:
                    LUI(div, imm);
                    break;
                case 0b10111:
                    AUIPC(div, imm);
                    break;
            }
        }

        void JAL(const Divide &div, const int32_t &imm) {
            if (div.rd) reg[div.rd] = pc;
            pc += imm - 4;
        }

        void J_type(const Divide &div) {
            int32_t imm = GetimmJ(div);
            JAL(div, imm);
        }

        void JALR(const Divide &div, const int32_t &imm) {
            uint32_t t = pc;
            pc = (reg[div.rs1] + imm) & (~1);
            if (div.rd) reg[div.rd] = t;
        }

        void LB(const Divide &div, const int32_t &imm) {
            reg[div.rd] = signedExtend<8>(memory.getPos8(reg[div.rs1] + imm));
        }//

        void LH(const Divide &div, const int32_t &imm) {
            reg[div.rd] = signedExtend<16>(memory.getPos16(reg[div.rs1] + imm));
        }//

        void LW(const Divide &div, const int32_t &imm) {
            reg[div.rd] = memory.getPos32(reg[div.rs1] + imm);
        }

        void LBU(const Divide &div, const int32_t &imm) {
            reg[div.rd] = memory.getPos8(reg[div.rs1] + imm);
        }//

        void LHU(const Divide &div, const int32_t &imm) {
            reg[div.rd] = memory.getPos16(reg[div.rs1] + imm);
        }//

        void ADDI(const Divide &div, const int32_t &imm) {
            reg[div.rd] = reg[div.rs1] + imm;
        }

        void SLTI(const Divide &div, const int32_t &imm) {
            if ((int32_t) reg[div.rs1] < imm) reg[div.rd] = 1; else reg[div.rd] = 0;
        }//

        void SLTIU(const Divide &div, const int32_t &imm) {
            if (reg[div.rs1] < (uint32_t) imm) reg[div.rd] = 1; else reg[div.rd] = 0;
        }//

        void XORI(const Divide &div, const int32_t &imm) {
            reg[div.rd] = reg[div.rs1] ^ imm;
        }//

        void ORI(const Divide &div, const int32_t &imm) {
            reg[div.rd] = reg[div.rs1] | imm;
        }//

        void ANDI(const Divide &div, const int32_t &imm) {
            reg[div.rd] = reg[div.rs1] & imm;
        }//

        void SLLI(const Divide &div) {
            reg[div.rd] = reg[div.rs1] << div.rs2;
        }

        void SRLI(const Divide &div) {
            reg[div.rd] = reg[div.rs1] >> div.rs2;
        }

        void SRAI(const Divide &div) {
            reg[div.rd] = ((int32_t) reg[div.rs1]) >> ((int32_t) div.rs2);
        }

        void I_type(Divide div) {
            int32_t imm = GetimmI(div);
            if (div.opcode == 0b1100111) {
                JALR(div, imm);
            } else if (div.opcode == 0b11) {
                switch (div.funct3) {
                    case 0b0:
                        LB(div, imm);
                        break;
                    case 0b1:
                        LH(div, imm);
                        break;
                    case 0b10:
                        LW(div, imm);
                        break;
                    case 0b100:
                        LBU(div, imm);
                        break;
                    case 0b101:
                        LHU(div, imm);
                        break;
                }
            } else if (div.opcode == 0b10011) {
                switch (div.funct3) {
                    case 0b0:
                        ADDI(div, imm);
                        break;
                    case 0b10:
                        SLTI(div, imm);
                        break;
                    case 0b11:
                        SLTIU(div, imm);
                        break;
                    case 0b100:
                        XORI(div, imm);
                        break;
                    case 0b110:
                        ORI(div, imm);
                        break;
                    case 0b111:
                        ANDI(div, imm);
                        break;
                    case 0b001:
                        SLLI(div);
                        break;
                    default:
                        switch (div.funct7) {
                            case 0b0:
                                SRLI(div);
                                break;
                            default:
                                SRAI(div);
                                break;
                        }
                        break;
                }
            }
        }

        void BEQ(const Divide &div, const int32_t &imm) {
            if (reg[div.rs1] == reg[div.rs2]) pc += imm - 4;
        }

        void BNE(const Divide &div, const int32_t &imm) {
            if (reg[div.rs1] != reg[div.rs2]) pc += imm - 4;
        }

        void BLT(const Divide &div, const int32_t &imm) {
            if ((int32_t) reg[div.rs1] < (int32_t) reg[div.rs2]) pc += imm - 4;
        }

        void BGE(const Divide &div, const int32_t &imm) {
            if ((int32_t) reg[div.rs1] >= (int32_t) reg[div.rs2]) pc += imm - 4;
        }

        void BLTU(const Divide &div, const int32_t &imm) {
            if (reg[div.rs1] < reg[div.rs2]) pc += imm - 4;
        }

        void BGEU(const Divide &div, const int32_t &imm) {
            if (reg[div.rs1] >= reg[div.rs2]) pc += imm - 4;
        }

        void B_type(Divide div) {
            int32_t imm = GetimmB(div);
            switch (div.funct3) {
                case 0b0:
                    BEQ(div, imm);
                    break;
                case 0b1:
                    BNE(div, imm);
                    break;
                case 0b100:
                    BLT(div, imm);
                    break;
                case 0b101:
                    BGE(div, imm);
                    break;
                case 0b110:
                    BLTU(div, imm);
                    break;
                case 0b111:
                    BGEU(div, imm);
                    break;
            }
        }

        void SB(const Divide &div, const int32_t &imm) {
            memory.getPos8(reg[div.rs1] + imm) = GetBitBetween(reg[div.rs2], 0, 7);
        }

        void SH(const Divide &div, const int32_t &imm) {
            memory.getPos16(reg[div.rs1] + imm) = GetBitBetween(reg[div.rs2], 0, 15);
        }//

        void SW(const Divide &div, const int32_t &imm) {
            memory.getPos32(reg[div.rs1] + imm) = reg[div.rs2];
        }

        void S_type(Divide div) {
            int32_t imm = GetimmS(div);
            switch (div.funct3) {
                case 0b0:
                    SB(div, imm);
                    break;
                case 0b1:
                    SH(div, imm);
                    break;
                case 0b10:
                    SW(div, imm);
                    break;
            }
        }

        void ADD(const Divide &div) {
            reg[div.rd] = reg[div.rs1] + reg[div.rs2];
        }//

        void SLL(const Divide &div) {
            reg[div.rd] = reg[div.rs1] << GetBitBetween(reg[div.rs2], 0, 4);
        }//

        void SLT(const Divide &div) {
            if ((int32_t) reg[div.rs1] < (int32_t) reg[div.rs2]) reg[div.rd] = 1; else reg[div.rd] = 0;
        }//

        void SLTU(const Divide &div) {
            if (reg[div.rs1] < reg[div.rs2]) reg[div.rd] = 1; else reg[div.rd] = 0;
        }//

        void XOR(const Divide &div) {
            reg[div.rd] = reg[div.rs1] ^ reg[div.rs2];
        }

        void SRL(const Divide &div) {
            reg[div.rd] = reg[div.rs1] >> GetBitBetween(reg[div.rs2], 0, 4);
        }//

        void OR(const Divide &div) {
            reg[div.rd] = reg[div.rs1] | reg[div.rs2];
        }

        void AND(const Divide &div) {
            reg[div.rd] = reg[div.rs1] & reg[div.rs2];
        }

        void SUB(const Divide &div) {
            reg[div.rd] = reg[div.rs1] - reg[div.rs2];
        }

        void SRA(const Divide &div) {
            reg[div.rd] = ((int32_t) reg[div.rs1]) >> ((int32_t) GetBitBetween(reg[div.rs2], 0, 4));
        }//

        void R_type(Divide div) {
            if (div.funct7 == 0b0) {
                switch (div.funct3) {
                    case 0b0:
                        ADD(div);
                        break;
                    case 0b1:
                        SLL(div);
                        break;
                    case 0b10:
                        SLT(div);
                        break;
                    case 0b11:
                        SLTU(div);
                        break;
                    case 0b100:
                        XOR(div);
                        break;
                    case 0b101:
                        SRL(div);
                        break;
                    case 0b110:
                        OR(div);
                        break;
                    case 0b111:
                        AND(div);
                        break;
                }
            } else {
                switch (div.funct3) {
                    case 0b0:
                        SUB(div);
                        break;
                    case 0b101:
                        SRA(div);
                        break;
                }
            }
        }
    };
}
#endif //RISCV_CPU_HPP
