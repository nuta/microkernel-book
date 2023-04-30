#pragma once
#include <libs/common/types.h>

// システムコール命令を発行する。
static inline uint32_t arch_syscall(uint32_t r0, uint32_t r1, uint32_t r2,
                                    uint32_t r3, uint32_t r4, uint32_t r5) {
    register int32_t a0 __asm__("a0") = r0;  // a0レジスタの内容
    register int32_t a1 __asm__("a1") = r1;  // a1レジスタの内容
    register int32_t a2 __asm__("a2") = r2;  // a2レジスタの内容
    register int32_t a3 __asm__("a3") = r3;  // a3レジスタの内容
    register int32_t a4 __asm__("a4") = r4;  // a4レジスタの内容
    register int32_t a5 __asm__("a5") = r5;  // a5レジスタの内容
    register int32_t result __asm__("a0");   // 戻り値 (a0レジスタに戻ってくる)

    // ecall命令を実行し、カーネルのシステムコールハンドラ (riscv32_trap_handler) に処理を移す。
    // 返り値がa0レジスタ (result変数) に戻ってくる。
    __asm__ __volatile__("ecall"
                         : "=r"(result)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5)
                         : "memory");
    return result;
}
