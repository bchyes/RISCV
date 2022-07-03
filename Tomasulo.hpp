//
// Created by 25693 on 2022/6/29.
//

#ifndef RISCV_TOMASULO_HPP
#define RISCV_TOMASULO_HPP

#include"CPU.hpp"

namespace RISCV {
    class Tomasulo : public CPU {
    private:

        static const uint8_t RS_SIZE = 32, FETCH_SIZE = 32, ROB_SIZE = 32, SLB_SIZE = 32;
        static const uint32_t INF32 = UINT32_MAX;

        template<class T, uint8_t SIZE>
        struct Queue {
            T val[SIZE]{};
            uint32_t head = 0, tail = 0;

            T &operator[](const uint32_t &pos) {
                return val[pos % SIZE];
            }

            T &front() {
                return val[head % SIZE];
            }

            void push(const T &value) {
                val[tail % SIZE] = value;
                ++tail;
            }

            void pop() {
                head++;
            }

            uint8_t size() {
                return (uint8_t) tail - head;
            }

            void clear() {
                head = tail = 0;
            }

            bool empty() {
                return size() == 0;
            }

            bool full() {
                return size() >= SIZE;
            }
        };

        struct Register_t {
            uint32_t reorder = 0, data = 0;
        };

        template<uint8_t SIZE = REG_SIZE>
        struct Register_for_tomasulo {
            Register_t val[SIZE]{};

            Register_t &operator[](const uint32_t &pos) {
                return val[pos];
            }
        };

        Register_for_tomasulo<> reg_prev, reg_next;

        struct ReservationStation_t {
            bool busy = false;
            Divide div;
            int32_t imm;
            uint32_t rs1_data, rs1_reorder, rs2_data, rs2_reorder, rd_reorder, pc, time;
        };

        /*template<uint8_t SIZE = RS_SIZE>
        struct ReservationStation {
            ReservationStation_t val[SIZE]{};

            ReservationStation_t &operator[](uint32_t pos) {
                return val[pos];
            }
        };*/

        Queue<ReservationStation_t, RS_SIZE> rs_prev, rs_next;

        struct Execute_t {
            uint32_t reg = 0, pc = 0;
            bool jump = false;
        };

        struct ReorderBuffer_t {
            Divide div;
            uint32_t reg, rs, slb;
            bool ready;
            Execute_t exe;
        };
        ReorderBuffer_t Issue_to_ROB;
        Queue<ReorderBuffer_t, ROB_SIZE> rob_prev, rob_next;
        struct SaveLoadBuffer_t {
            Divide div;
            int32_t imm;
            uint32_t rs1_data, rs1_reorder, rs2_data, rs2_reorder, rd_reorder, time;
            //bool ready = false;
        };
        SaveLoadBuffer_t Issue_to_SLB;//data用于记录寄存器编号
        Queue<SaveLoadBuffer_t, SLB_SIZE> slb_prev, slb_next;

        struct FetchQueue_t {
            uint32_t command, pc;
        };

        Queue<FetchQueue_t, FETCH_SIZE> Fetch_prev, Fetch_next;
        FetchQueue_t Fetch_to_Issue;

        struct Issue_to_Regfile_t {
            uint32_t reg, reorder;
        };
        Issue_to_Regfile_t Issue_to_Regfile;

        struct Issue_to_RS_t {
            Divide div;
            int32_t imm;
            uint32_t rs1, rs2, time, pc, reorder_id, rs_id;
        };
        Issue_to_RS_t Issue_to_RS;

        struct RS_to_EX_t {
            Divide div;
            int32_t imm;
            uint32_t rs1_data, rs2_data, pc, rob_id, time;
        };
        RS_to_EX_t RS_to_EX;

        struct EX_to_ROB_t {
            uint32_t reorder;
            Execute_t exe;
        };
        EX_to_ROB_t EX_to_ROB, SLB_to_ROB_prev, SLB_to_ROB_next;
        struct ROB_to_commit_t {
            Divide div;
            uint32_t reg_id, reorder_id, slb_id;
            Execute_t exe;
        };
        ROB_to_commit_t ROB_to_commit;
        struct EX_to_RS_t {
            uint32_t reorder, reg;
        };
        EX_to_RS_t EX_to_RS, EX_to_SLB, SLB_to_SLB_prev, SLB_to_SLB_next, SLB_to_RS_prev, SLB_to_RS_next;
        struct commit_to_regfile_t {
            uint32_t reorder_id, reg_id, data;
        };
        commit_to_regfile_t commit_to_regfile;
        struct commit_to_SLB_t {
            uint32_t slb_id;
        };
        commit_to_SLB_t commit_to_SLB;

        bool stall_fetch_queue = true, stall_fetch_to_issue = true;
        bool stall_issue_to_fetch = true, stall_issue_to_rob = true, stall_issue_to_rs = true, stall_issue_to_slb = true, stall_issue_to_regfile = true;
        bool stall_rs_to_ex = true;
        bool stall_ex_to_rob = true, stall_ex_to_rs = true, stall_ex_to_slb = true;
        bool stall_slb_to_rob_prev = true, stall_slb_to_rs_prev = true, stall_slb_to_slb_prev = true;
        bool stall_slb_to_rob_next = true, stall_slb_to_rs_next = true, stall_slb_to_slb_next = true;
        bool stall_rob_to_commit = true;
        bool stall_commit_to_regfile = true, stall_commit_to_slb = true;
        bool stall_clear_rob = true, stall_clear_slb = true, stall_clear_rs = true, stall_clear_fq = true, stall_clear_regfile = true;
    public:

        void RunTomasulo() {
            for (int cycle = 0; cycle <= 9; cycle++) {
                /*在这里使用了两阶段的循环部分：
                  1. 实现时序电路部分，即在每个周期初同步更新的信息。
                  2. 实现逻辑电路部分，即在每个周期中如ex、issue的部分
                  已在下面给出代码
                */
                update();

                run_rob();
                /*if(code_from_rob_to_commit == 0x0ff00513){
                    std::cout << std::dec << ((unsigned int)reg_prev[10] & 255u);
                    break;
                }*/
                run_slbuffer();
                run_reservation();
                run_regfile();
                run_inst_fetch_queue();

                run_ex();
                run_issue();
                run_commit();
            }
            for (int cycle = 10;; cycle++) {
                /*在这里使用了两阶段的循环部分：
                  1. 实现时序电路部分，即在每个周期初同步更新的信息。
                  2. 实现逻辑电路部分，即在每个周期中如ex、issue的部分
                  已在下面给出代码
                */
                update();

                run_rob();
                /*if(code_from_rob_to_commit == 0x0ff00513){
                    std::cout << std::dec << ((unsigned int)reg_prev[10] & 255u);
                    break;
                }*/
                run_slbuffer();
                run_reservation();
                run_regfile();
                run_inst_fetch_queue();

                run_ex();
                run_issue();
                run_commit();

                if (!stall_rob_to_commit && ROB_to_commit.div.command == 0x0ff00513) {
                    printf("%u\n", GetBitBetween(reg_prev[10].data, 0, 7));
                    break;
                }
            }
        }

        void run_inst_fetch_queue() {
            /*
            在这一部分你需要完成的工作：
            1. 实现一个先进先出的指令队列
            2. 读取指令并存放到指令队列中
            3. 准备好下一条issue的指令
            tips: 考虑边界问题（满/空...）
            */
            stall_fetch_to_issue = true;
            if (!stall_clear_fq) {
                Fetch_next.clear();
                return;
            }
            if (!Fetch_prev.full()) {
                Fetch_next.push({GetCommand(pc), pc - 4});
            }
            if (!stall_issue_to_fetch) {
                if (Fetch_prev.size() > 1) {
                    Fetch_to_Issue = Fetch_prev[Fetch_prev.head + 1];
                    stall_fetch_to_issue = false;
                }
                Fetch_next.pop();
            } else {
                if (!Fetch_prev.empty()) {
                    Fetch_to_Issue = Fetch_prev.front();
                    stall_fetch_to_issue = false;
                }
            }
        }

        void run_issue() {
            /*
            在这一部分你需要完成的工作：
            1. 从run_inst_fetch_queue()中得到issue的指令
            2. 对于issue的所有类型的指令向ROB申请一个位置（或者也可以通过ROB预留位置），并修改regfile中相应的值
            2. 对于 非 Load/Store的指令，将指令进行分解后发到Reservation Station
              tip: 1. 这里需要考虑怎么得到rs1、rs2的值，并考虑如当前rs1、rs2未被计算出的情况，参考书上内容进行处理
                   2. 在本次作业中，我们认为相应寄存器的值已在ROB中存储但尚未commit的情况是可以直接获得的，即你需要实现这个功能
                      而对于rs1、rs2不ready的情况，只需要stall即可，有兴趣的同学可以考虑下怎么样直接从EX完的结果更快的得到计算结果
            3. 对于 Load/Store指令，将指令分解后发到SLBuffer(需注意SLBUFFER也该是个先进先出的队列实现)
            tips: 考虑边界问题（是否还有足够的空间存放下一条指令）
            */
            stall_issue_to_fetch = stall_issue_to_regfile = stall_issue_to_rob = stall_issue_to_slb = stall_issue_to_rs = true;
            if (!stall_fetch_to_issue) {
                if (!rob_prev.full()) {//!!
                    Divide div = GetDivide(Fetch_to_Issue.command);
                    int32_t imm = Getimm(div);
                    uint32_t nextrs = RS_SIZE;
                    uint32_t nextslb = slb_prev.tail;
                    if (div.opcode == 0b11 || div.opcode == 0b100011) {
                        if (!slb_prev.full()) {//手误！号没加
                            stall_issue_to_fetch = stall_issue_to_slb = stall_issue_to_rob = false;
                            Issue_to_SLB = {div, imm, div.rs1, 0, div.rs2, 0, rob_next.tail, 3};
                            Issue_to_ROB = {div, div.rd, 0, nextslb, false};
                            if (div.opcode == 0b11) {
                                if (div.rd != 0) {
                                    stall_issue_to_regfile = false;
                                    Issue_to_Regfile = {div.rd, rob_next.tail};
                                }
                            }
                        }
                    } else {
                        for (int i = 0; i < RS_SIZE; i++) {
                            if (!rs_next[i].busy) {
                                nextrs = i;
                                break;
                            }
                        }
                        if (nextrs != RS_SIZE) {
                            stall_issue_to_fetch = stall_issue_to_rob = stall_issue_to_rs = false;
                            Issue_to_RS = {div, imm, div.rs1, div.rs2, 1, Fetch_to_Issue.pc, rob_next.tail, nextrs};
                            Issue_to_ROB = {div, div.rd, nextrs, 0, false};
                            if (div.opcode != 0b1100011 && div.rd != 0) {//非B操作不需对寄存器操作
                                stall_issue_to_regfile = false;
                                Issue_to_Regfile = {div.rd, rob_next.tail};
                            }
                        }
                    }
                }
            }
        }

        void run_regfile() {
            if (!stall_commit_to_regfile && commit_to_regfile.reg_id) {
                auto &reg = reg_next[commit_to_regfile.reg_id];
                reg.data = commit_to_regfile.data;
                if (reg.reorder == commit_to_regfile.reorder_id) {
                    reg.reorder = 0;
                }//可能先修改reorder但是还是没有修改data
            }
            if (!stall_clear_regfile) {
                for (int i = 0; i < REG_SIZE; i++) {
                    reg_next[i].reorder = 0;
                }
                return;
            }
            if (!stall_issue_to_regfile)
                reg_next[Issue_to_Regfile.reg].reorder = Issue_to_Regfile.reorder;
        }

        void run_reservation() {
            /*
            在这一部分你需要完成的工作：
            1. 设计一个Reservation Station，其中要存储的东西可以参考CAAQA或其余资料，至少需要有用到的寄存器信息等
            2. 如存在，从issue阶段收到要存储的信息，存进Reservation Station（如可以计算也可以直接进入计算）
            3. 从Reservation Station或者issue进来的指令中选择一条可计算的发送给EX进行计算
            4. 根据上个周期EX阶段或者SLBUFFER的计算得到的结果遍历Reservation Station，更新相应的值
            */
            if (!stall_clear_rs) {
                stall_rs_to_ex = true;
                for (uint32_t i = 0; i < RS_SIZE; i++) {
                    rs_next[i].busy = false;
                }
                return;
            }
            if (!stall_issue_to_rs) {
                auto &next = rs_next[Issue_to_RS.rs_id];
                next.busy = true;
                next.div = Issue_to_RS.div;
                next.imm = Issue_to_RS.imm;
                next.time = Issue_to_RS.time;
                next.rd_reorder = Issue_to_RS.reorder_id;
                next.pc = Issue_to_RS.pc;
                //////////
                if (reg_prev[Issue_to_RS.div.rs1].reorder == 0) {
                    next.rs1_reorder = 0;
                    next.rs1_data = reg_prev[Issue_to_RS.div.rs1].data;
                } else {
                    auto reorder_id = reg_prev[Issue_to_RS.div.rs1].reorder;
                    if (!stall_ex_to_rs && EX_to_RS.reorder == reorder_id) {
                        next.rs1_reorder = 0;
                        next.rs1_data = EX_to_RS.reg;
                    } else if (!stall_slb_to_rs_prev && SLB_to_RS_prev.reorder == reorder_id) {
                        next.rs1_reorder = 0;
                        next.rs1_data = SLB_to_RS_prev.reg;
                    } else if (rob_prev[reorder_id].ready) {
                        next.rs1_reorder = 0;
                        next.rs1_data = rob_prev[reorder_id].exe.reg;//此reg为值
                    } else
                        next.rs1_reorder = reorder_id;
                }
                if (reg_prev[Issue_to_RS.div.rs2].reorder == 0) {
                    next.rs2_reorder = 0;
                    next.rs2_data = reg_prev[Issue_to_RS.div.rs2].data;
                } else {
                    auto reorder_id = reg_prev[Issue_to_RS.div.rs2].reorder;
                    if (!stall_ex_to_rs && EX_to_RS.reorder == reorder_id) {
                        next.rs2_reorder = 0;
                        next.rs2_data = EX_to_RS.reg;
                    } else if (!stall_slb_to_rs_prev && SLB_to_RS_prev.reorder == reorder_id) {
                        next.rs2_reorder = 0;
                        next.rs2_data = SLB_to_RS_prev.reg;
                    } else if (rob_prev[reorder_id].ready) {
                        next.rs2_reorder = 0;
                        next.rs2_data = rob_prev[reorder_id].exe.reg;//此reg为值
                    } else
                        next.rs2_reorder = reorder_id;
                }
            }
            stall_rs_to_ex = true;
            for (int i = 0; i < RS_SIZE; i++) {
                if (rs_prev[i].busy) {
                    OperationType type = GetType(rs_prev[i].div);
                    bool flag = 0;
                    switch (type) {
                        case R:
                        case B:
                            if (rs_prev[i].rs1_reorder == 0 && rs_prev[i].rs2_reorder == 0) {
                                flag = true;
                            }
                            break;
                        case I:
                            if (rs_prev[i].rs1_reorder == 0) {
                                flag = true;
                            }
                            break;
                        case U:
                        case J:
                            flag = true;
                            break;
                    }
                    if (flag) {
                        stall_rs_to_ex = false;
                        RS_to_EX = {rs_prev[i].div, rs_prev[i].imm, rs_prev[i].rs1_data, rs_prev[i].rs2_data,
                                    rs_prev[i].pc, rs_prev[i].rd_reorder, rs_prev[i].time};
                        rs_next[i].busy = false;
                        break;
                    }
                }
            }
        }

        void run_ex() {
            /*
            在这一部分你需要完成的工作：
            根据Reservation Station发出的信息进行相应的计算
            tips: 考虑如何处理跳转指令并存储相关信息
                  Store/Load的指令并不在这一部分进行处理
            */
            stall_ex_to_rob = stall_ex_to_rs = stall_ex_to_slb = true;
            if (!stall_rs_to_ex) {
                --RS_to_EX.time;
                if (!RS_to_EX.time) {
                    Execute_t tmp = execute(RS_to_EX.div, RS_to_EX.imm, RS_to_EX.rs1_data, RS_to_EX.rs2_data,
                                            RS_to_EX.pc);
                    stall_ex_to_rob = stall_ex_to_rs = stall_ex_to_slb = false;
                    EX_to_ROB = {RS_to_EX.rob_id, tmp};
                }
            }
        }

        void run_slbuffer() {
            /*
            在这一部分中，由于SLBUFFER的设计较为多样，在这里给出两种助教的设计方案：
            1. 1）通过循环队列，设计一个先进先出的SLBUFFER，同时存储 head1、head2、tail三个变量。
               其中，head1是真正的队首，记录第一条未执行的内存操作的指令；tail是队尾，记录当前最后一条未执行的内存操作的指令。
               而head2负责确定处在head1位置的指令是否可以进行内存操作，其具体实现为在ROB中增加一个head_ensure的变量，每个周期head_ensure做取模意义下的加法，直到等于tail或遇到第一条跳转指令，
               这个时候我们就可以保证位于head_ensure及之前位置的指令，因中间没有跳转指令，一定会执行。因而，只要当head_ensure当前位置的指令是Store、Load指令，我们就可以向slbuffer发信息，增加head2。
               简单概括即对head2之前的Store/Load指令，我们根据判断出ROB中该条指令之前没有jump指令尚未执行，从而确定该条指令会被执行。
               2）同时SLBUFFER还需根据上个周期EX和SLBUFFER的计算结果遍历SLBUFFER进行数据的更新。
               3）此外，在我们的设计中，将SLBUFFER进行内存访问时计算需要访问的地址和对应的读取/存储内存的操作在SLBUFFER中一并实现，
               也可以考虑分成两个模块，该部分的实现只需判断队首的指令是否能执行并根据指令相应执行即可。
            2. 1）SLB每个周期会查看队头，若队头指令还未ready，则阻塞。

               2）当队头ready且是load指令时，SLB会直接执行load指令，包括计算地址和读内存，
               然后把结果通知ROB，同时将队头弹出。ROB commit到这条指令时通知Regfile写寄存器。

               3）当队头ready且是store指令时，SLB会等待ROB的commit，commit之后会SLB执行这
               条store指令，包括计算地址和写内存，写完后将队头弹出。
            */
            if (!stall_issue_to_slb) {
                SaveLoadBuffer_t next = Issue_to_SLB;
                Register_t reg = reg_prev[next.rs1_data];
                if (reg.reorder == 0) {
                    next.rs1_reorder = 0;
                    next.rs1_data = reg.data;
                } else {
                    if (!stall_ex_to_slb && EX_to_SLB.reorder == reg.reorder) {
                        next.rs1_reorder = 0;
                        next.rs1_data = EX_to_SLB.reg;
                    } else if (!stall_slb_to_slb_prev && SLB_to_SLB_prev.reorder == reg.reorder) {
                        next.rs1_reorder = 0;
                        next.rs1_data = SLB_to_SLB_prev.reg;
                    } else if (rob_prev[reg.reorder].ready) {
                        next.rs1_reorder = 0;
                        next.rs1_data = rob_prev[reg.reorder].exe.reg;
                    } else {
                        next.rs1_reorder = reg.reorder;
                    }
                }
                reg = reg_prev[next.rs2_data];
                if (reg.reorder == 0) {
                    next.rs2_reorder = 0;
                    next.rs2_data = reg.data;
                } else {
                    if (!stall_ex_to_slb && EX_to_SLB.reorder == reg.reorder) {
                        next.rs2_reorder = 0;
                        next.rs2_data = EX_to_SLB.reg;
                    } else if (!stall_slb_to_slb_prev && SLB_to_SLB_prev.reorder == reg.reorder) {
                        next.rs2_reorder = 0;
                        next.rs2_data = SLB_to_SLB_prev.reg;
                    } else if (rob_prev[reg.reorder].ready) {
                        next.rs2_reorder = 0;
                        next.rs2_data = rob_prev[reg.reorder].exe.reg;
                    } else {
                        next.rs2_reorder = reg.reorder;
                    }
                }
                slb_next.push(next);
            }
            if (!stall_ex_to_slb) {
                for (uint32_t i = slb_prev.head; i <= slb_next.head; i++) {
                    if (slb_prev[i].rs1_reorder == SLB_to_SLB_prev.reorder) {
                        slb_next[i].rs1_reorder = 0;
                        slb_next[i].rs1_data = EX_to_SLB.reg;
                    }
                    if (slb_prev[i].rs1_reorder == SLB_to_SLB_prev.reorder) {
                        slb_next[i].rs1_reorder = 0;
                        slb_next[i].rs1_data = EX_to_SLB.reg;
                    }
                }
            }
            if (!stall_commit_to_slb) {
                slb_next[commit_to_SLB.slb_id].rd_reorder = INF32;
                --slb_next[commit_to_SLB.slb_id].time;
            }//?
            stall_slb_to_rob_next = stall_slb_to_rs_next = stall_slb_to_slb_next = true;
            if (!slb_prev.empty()) {
                const auto &front = slb_prev.front();
                if (front.div.opcode == 0b11) {
                    if (front.rs1_reorder == 0) {
                        if (front.time == 1) {
                            Execute_t tmp = execute(front.div, front.imm, front.rs1_data, front.rs2_data, 0);
                            SLB_to_SLB_next = {front.rd_reorder, tmp.reg};
                            SLB_to_RS_next = {front.rd_reorder, tmp.reg};
                            SLB_to_ROB_next = {front.rd_reorder, tmp};
                            //?
                            stall_slb_to_rob_next = stall_slb_to_rs_next = stall_slb_to_slb_next = true;
                            slb_next.pop();
                        } else {
                            --slb_next.front().time;
                        }
                    }
                } else {
                    //?
                    if (stall_commit_to_slb || commit_to_SLB.slb_id != slb_prev.head) {
                        if (front.rs1_reorder == 0 && front.rs2_reorder == 0) {
                            //?
                            if (front.rd_reorder != INF32) {
                                stall_slb_to_rob_next = false;
                                SLB_to_ROB_next = {front.rd_reorder};
                            } else {
                                if (front.time == 1) {
                                    auto x = front.rs1_data + front.imm, y = front.rs2_data;
                                    switch (front.div.funct3) { //sb sh sw
                                        case 0:
                                            memory.getPos8(x) = GetBitBetween(y, 0, 7);
                                            break;
                                        case 1:
                                            memory.getPos16(x) = GetBitBetween(y, 0, 15);
                                            break;
                                        case 2:
                                            memory.getPos32(x) = y;
                                            break;
                                    }
                                    slb_next.pop();
                                } else {
                                    --slb_next.front().time;
                                }
                            }
                        }
                    }
                }
            }
            //?
            if (!stall_clear_slb) {
                if (slb_prev.front().rd_reorder == INF32 && slb_prev.front().time > 1) {
                    slb_next.tail = slb_next.head + 1;
                } else {
                    slb_next.clear();
                }
                stall_slb_to_rob_next = stall_slb_to_slb_next = stall_slb_to_rs_next = true;
                return;
            }
        }

        void run_rob() {
            /*
            在这一部分你需要完成的工作：
            1. 实现一个先进先出的ROB，存储所有指令
            1. 根据issue阶段发射的指令信息分配空间进行存储。
            2. 根据EX阶段和SLBUFFER的计算得到的结果，遍历ROB，更新ROB中的值
            3. 对于队首的指令，如果已经完成计算及更新，进行commit
            */
            if (!stall_clear_rob) {
                rob_next.clear();
                stall_rob_to_commit = true;
                return;
            }
            if (!stall_issue_to_rob) {
                ReorderBuffer_t next = Issue_to_ROB;
                next.ready = false;
                rob_next.push(next);
            }
            //!
            if (!stall_ex_to_rob) {
                rob_next[EX_to_ROB.reorder].ready = true;
                rob_next[EX_to_ROB.reorder].exe = EX_to_ROB.exe;
            }
            if (!stall_slb_to_rob_prev) {
                rob_next[SLB_to_ROB_prev.reorder].ready = true;
                rob_next[SLB_to_ROB_prev.reorder].exe = SLB_to_ROB_prev.exe;
            }

            stall_rob_to_commit = true;
            if (!rob_prev.empty() && rob_prev.front().ready) {
                ReorderBuffer_t front = rob_prev.front();
                ROB_to_commit = {front.div, front.reg, rob_prev.head, front.slb, front.exe};
                stall_rob_to_commit = false;
                rob_next.pop();
            }
        }

        void run_commit() {
            /*
            在这一部分你需要完成的工作：
            1. 根据ROB发出的信息更新寄存器的值，包括对应的ROB和是否被占用状态（注意考虑issue和commit同一个寄存器的情况）
            2. 遇到跳转指令更新pc值，并发出信号清空所有部分的信息存储（这条对于很多部分都有影响，需要慎重考虑）
            */
            stall_commit_to_regfile = stall_commit_to_regfile = true;
            stall_clear_fq = stall_clear_regfile = stall_clear_rob = stall_clear_rs = stall_clear_slb = true;
            if (!stall_rob_to_commit) {
                OperationType type = GetType(ROB_to_commit.div);
                if (type != B && type != S) {
                    stall_commit_to_regfile = false;
                    commit_to_regfile = {ROB_to_commit.reorder_id, ROB_to_commit.reg_id, ROB_to_commit.exe.reg};
                }//!jal需要两个
                if ((type == B || ROB_to_commit.div.opcode == 0b1101111 ||
                     ROB_to_commit.div.opcode == 0b1100111) && ROB_to_commit.exe.jump) {//jump: jal jalr bxx
                    stall_clear_fq = stall_clear_regfile = stall_clear_rob = stall_clear_rs = stall_clear_slb = false;
                    pc = ROB_to_commit.exe.pc;
                }
                if (type == S) {
                    stall_commit_to_slb = false;
                    commit_to_SLB = {ROB_to_commit.slb_id};
                }
            }
        }

        void update() {
            /*
            在这一部分你需要完成的工作：
            对于模拟中未完成同步的变量（即同时需记下两个周期的新/旧信息的变量）,进行数据更新。
            */
            Fetch_prev = Fetch_next;
            reg_prev = reg_next;
            rs_prev = rs_next;
            slb_prev = slb_next;
            rob_prev = rob_next;
            SLB_to_ROB_prev = SLB_to_ROB_next;
            SLB_to_SLB_prev = SLB_to_SLB_next;
            SLB_to_RS_prev = SLB_to_RS_next;
            stall_slb_to_slb_prev = stall_slb_to_slb_next;
            stall_slb_to_rs_prev = stall_slb_to_rs_next;
            stall_slb_to_rob_prev = stall_slb_to_rob_next;
        }

        Execute_t execute(Divide div, int32_t imm, uint32_t v1, uint32_t v2, uint32_t pc) {
            switch (div.opcode) {
                case 0x03: { //load: lb lh lw lbu lhu
                    switch (div.funct3) {
                        case 0:
                            return {(uint32_t) signedExtend<8>(memory.getPos8(v1 + imm)), 0, false};
                        case 1:
                            return {(uint32_t) signedExtend<16>(memory.getPos16(v1 + imm)), 0, false};
                        case 2:
                            return {memory.getPos32(v1 + imm), 0, false};
                        case 4:
                            return {memory.getPos8(v1 + imm), 0, false};
                        case 5:
                            return {memory.getPos16(v1 + imm), 0, false};
                    }
                }
                case 0x23: { //sb sh sw
                    switch (div.funct3) {
                        case 0:
                            return {0, 0, false};
                        case 1:
                            return {0, 0, false};
                        case 2:
                            return {0, 0, false};
                    }
                }
                case 0x33: {
                    switch (div.funct3) { //add sub sll slt sltu xor srl sra
                        case 0: //add sub
                            if (div.funct7 == 0) return {v1 + v2, 0, false};
                            else return {v1 - v2, 0, false};
                        case 1: //sll
                            return {v1 << GetBitBetween(v2, 0, 4), 0, false};
                        case 2: //slt sltu
                            return {(int32_t) (v1) < (int32_t) (v2), 0, false};
                        case 3: //slt
                            return {v1 < v2, 0, false};
                        case 4: //xor
                            return {v1 ^ v2, 0, false};
                        case 5: //srl sra
                            if (div.funct7 == 0) return {v1 >> GetBitBetween(v2, 0, 4), 0, false};
                            else
                                return {(uint32_t) ((int32_t) (v1) >> (int32_t) (GetBitBetween(v2, 0, 4))), 0, false};
                        case 6: //or and
                            return {v1 | v2, 0, false};
                        case 7:
                            return {v1 & v2, 0, false};
                    }
                }
                case 0x13: {
                    switch (div.funct3) { //addi slti sltiu xori ori andi slli srli srai
                        case 0:
                            return {v1 + imm, 0, false};
                        case 2:
                            return {(int32_t) (v1) < imm, 0, false};
                        case 3:
                            return {v1 < (uint32_t) (imm), 0, false};
                        case 4:
                            return {v1 ^ imm, 0, false};
                        case 6:
                            return {v1 | imm, 0, false};
                        case 7:
                            return {v1 & imm, 0, false};
                        case 1:
                            return {v1 << div.rs2, 0, false};
                        case 5:
                            if (div.funct7 == 0) return {v1 >> div.rs2, 0, false};
                            else
                                return {(uint32_t) ((int32_t) (v1) >> (int32_t) (div.rs2)), 0, false};
                    }
                }
                case 0x37:
                    return {(uint32_t) imm, 0, false}; //lui
                case 0x17:
                    return {pc + imm, 0, false}; //auipc
                case 0x6F:
                    return {pc + 4, pc + imm, true}; //jal; assert(rd > 0)
                case 0x67:
                    return {pc + 4, ((v1 + imm) & (~1u)), true}; //jalr; assert rd>0; write after read
                case 0x63: {
                    switch (div.funct3) { //beq bne blt bge bltu bgeu
                        case 0:
                            return {0, pc + imm, v1 == v2};
                        case 1:
                            return {0, pc + imm, v1 != v2};
                        case 4:
                            return {0, pc + imm, (int32_t) (v1) < (int32_t) (v2)};
                        case 5:
                            return {0, pc + imm, (int32_t) (v1) >= (int32_t) (v2)};
                        case 6:
                            return {0, pc + imm, v1 < v2};
                        case 7:
                            return {0, pc + imm, v1 >= v2};
                    }
                }
                    /*case 0:
                    default: { //error
                        return (execute_t) {INF32, 0, false};
                    }*/
            }
        }
    };
}
#endif //RISCV_TOMASULO_HPP
