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
    //freopen("naive.data", "r", stdin);
    RISCV::run();
    return 0;
}
