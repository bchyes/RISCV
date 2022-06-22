#include "CPU.hpp"
#include<iostream>

namespace RISCV {
    void run() {
        CPU cpu;
        cpu.ReadMemory(stdin);
        cpu.Run();
    }
}

int main() {
    //freopen("lvalue2.data", "r", stdin);
    RISCV::run();
    return 0;
}
