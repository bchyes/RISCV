#include "CPU.hpp"
#include "CPU_SINGLE_PIPELINE.hpp"
#include "CPU_FIVE_PIPLINE.hpp"
#include <iostream>

namespace RISCV {
    void Run() {
        CPU_SINGLE_PIPELINE cpu;
        cpu.ReadMemory(stdin);
        cpu.Run();
    }

    void RunFiveStagePipeline() {
        CPU_FIVE_PIPELINE cpu;
        cpu.ReadMemory(stdin);
        cpu.RunFiveStagePipeline();
    }
}

int main() {
    //freopen("pi.data", "r", stdin);
    RISCV::RunFiveStagePipeline();
    return 0;
}
