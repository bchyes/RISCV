#include "CPU.hpp"
#include <iostream>

namespace RISCV {
    void run() {
        CPU cpu;
        cpu.ReadMemory(stdin);
        cpu.Run();
    }
}

int main() {
    RISCV::run();
    return 0;
}
