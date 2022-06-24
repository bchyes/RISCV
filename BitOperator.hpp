//
// Created by 25693 on 2022/6/21.
//

#ifndef RISCV_BITOPERATOR_HPP
#define RISCV_BITOPERATOR_HPP
namespace RISCV {
    union Divide {
        uint32_t command;
        struct {
            unsigned opcode: 7;
            unsigned rd: 5;
            unsigned funct3: 3;
            unsigned rs1: 5;
            unsigned rs2: 5;
            unsigned funct7: 7;
        };

        explicit Divide() = default;

        explicit Divide(const uint32_t &value) : command(value) {};
    };

    template<typename T, uint32_t B>
    T signExtend(const T &val) {
        struct {
            T x: B;
        } s;
        s.x = val;
        return s.x;
    }

    template<uint32_t B>
    int32_t signedExtend(const int32_t &val) {
        return signExtend<int32_t, B>(val);
    }

    int32_t GetimmU(const Divide &div) {
        return (int32_t) ((div.command >> 12) << 12);
    }

    uint32_t GetBitBetween(const uint32_t &val, const uint8_t &l, const uint8_t &r) {
        return (val >> l) & ((1 << (r - l + 1)) - 1);
    }

    int32_t GetimmJ(const Divide &div) {
        return signedExtend<21>(
                (int32_t) ((GetBitBetween(div.command, 21, 30) << 1) | (GetBitBetween(div.command, 20, 20) << 11) |
                           (GetBitBetween(div.command, 12, 19) << 12) |
                           (GetBitBetween(div.command, 31, 31) << 20)));
    }

    int32_t GetimmI(const Divide &div) {
        return signedExtend<12>(div.command >> 20);
    }

    int32_t GetimmS(const Divide &divide) {
        return signedExtend<12>((int32_t) ((divide.funct7 << 5) | divide.rd));
    }

    int32_t GetimmB(const Divide &div) {
        return signedExtend<13>(
                (int32_t) ((GetBitBetween(div.command, 8, 11) << 1) | (GetBitBetween(div.command, 25, 30) << 5) |
                           (GetBitBetween(div.command, 7, 7) << 11) |
                           (GetBitBetween(div.command, 31, 31) << 12)));
    }

    Divide GetDivide(const uint32_t &command) {
        return Divide(command);
    }
}
#endif //RISCV_BITOPERATOR_HPP
