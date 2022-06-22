#include "CPU.hpp"
#include<iostream>

namespace RISCV {
    void run() {
        CPU cpu;
        //cpu.ReadMemory(stdin);
        cpu.ReadMemoryForTest();
        cpu.Run();
    }
}

int main() {
    //freopen("lvalue2.data", "r", stdin);
    RISCV::run();
    return 0;
}
