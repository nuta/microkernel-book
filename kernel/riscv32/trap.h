#pragma once
#include <libs/common/types.h>

// 割り込み・例外発生時の実行状態。riscv32_trap_handler関数で保存・復元される。
struct riscv32_trap_frame {
    uint32_t pc;       // オフセット: 4 * 0
    uint32_t sstatus;  // オフセット: 4 * 1
    uint32_t ra;       // オフセット: 4 * 2
    uint32_t sp;       // オフセット: 4 * 3
    uint32_t gp;       // オフセット: 4 * 4
    uint32_t tp;       // オフセット: 4 * 5
    uint32_t t0;       // オフセット: 4 * 6
    uint32_t t1;       // オフセット: 4 * 7
    uint32_t t2;       // オフセット: 4 * 8
    uint32_t t3;       // オフセット: 4 * 9
    uint32_t t4;       // オフセット: 4 * 10
    uint32_t t5;       // オフセット: 4 * 11
    uint32_t t6;       // オフセット: 4 * 12
    uint32_t a0;       // オフセット: 4 * 13
    uint32_t a1;       // オフセット: 4 * 14
    uint32_t a2;       // オフセット: 4 * 15
    uint32_t a3;       // オフセット: 4 * 16
    uint32_t a4;       // オフセット: 4 * 17
    uint32_t a5;       // オフセット: 4 * 18
    uint32_t a6;       // オフセット: 4 * 19
    uint32_t a7;       // オフセット: 4 * 20
    uint32_t s0;       // オフセット: 4 * 21
    uint32_t s1;       // オフセット: 4 * 22
    uint32_t s2;       // オフセット: 4 * 23
    uint32_t s3;       // オフセット: 4 * 24
    uint32_t s4;       // オフセット: 4 * 25
    uint32_t s5;       // オフセット: 4 * 26
    uint32_t s6;       // オフセット: 4 * 27
    uint32_t s7;       // オフセット: 4 * 28
    uint32_t s8;       // オフセット: 4 * 29
    uint32_t s9;       // オフセット: 4 * 30
    uint32_t s10;      // オフセット: 4 * 31
    uint32_t s11;      // オフセット: 4 * 32
} __packed;

void riscv32_handle_trap(struct riscv32_trap_frame *frame);
