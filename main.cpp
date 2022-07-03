#include "CPU.hpp"
#include "CPU_SINGLE_PIPELINE.hpp"
#include "CPU_FIVE_PIPLINE.hpp"
#include "Tomasulo.hpp"
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

    void RunTomasulo() {
        Tomasulo cpu;
        cpu.ReadMemory(stdin);
        cpu.RunTomasulo();
    }
}

int main() {
    //freopen("naive.data", "r", stdin);
    RISCV::RunFiveStagePipeline();
    return 0;
}
