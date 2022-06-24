//
// Created by 25693 on 2022/6/21.
//

#ifndef RISCV_CPU_HPP
#define RISCV_CPU_HPP

#include <cstdio>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <queue>
#include "BitOperator.hpp"
namespace RISCV {
    class CPU {
    protected:
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
    };

}
#endif //RISCV_CPU_HPP
