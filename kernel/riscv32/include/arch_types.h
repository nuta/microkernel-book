#pragma once
#include "../asmdefs.h"
#include <libs/common/types.h>

// 仮想アドレス空間上のカーネルメモリ領域の開始アドレス。
#define KERNEL_BASE 0x80000000
// IRQの最大数。
#define IRQ_MAX 32

// RISC-V特有のタスク管理構造体。
struct arch_task {
    uint32_t sp;        // 次回実行時に復元されるべきカーネルスタックの値
    uint32_t sp_top;    // カーネルスタックの上端
    paddr_t sp_bottom;  // カーネルスタックの底
};

// RISC-V特有のページテーブル管理構造体。
struct arch_vm {
    paddr_t table;  // ページテーブルの物理アドレス (Sv32)
};

// RISC-V特有のCPUローカル変数。順番を変える時はasmdefs.hで定義しているマクロも更新する。
struct arch_cpuvar {
    uint32_t sscratch;  // 変数の一時保管場所
    uint32_t sp_top;    // 実行中タスクのカーネルスタックの上端

    // タイマー割り込みハンドラ (M-mode) で使用。
    uint32_t mscratch0;   // 変数の一時保管場所
    uint32_t mscratch1;   // 変数の一時保管場所その2
    paddr_t mtimecmp;     // MTIMECMPのアドレス
    paddr_t mtime;        // MTIMEのアドレス
    uint32_t interval;    // MTIMECMPに加算していく値
    uint64_t last_mtime;  // 直前のmtimeの値
};

// CPUVAR_* マクロが正しく定義されているかをチェックするためのマクロ。
#define ARCH_TYPES_STATIC_ASSERTS                                              \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.sscratch) == CPUVAR_SSCRATCH,   \
                  "CPUVAR_SSCRATCH is incorrect");                             \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.sp_top) == CPUVAR_SP_TOP,       \
                  "CPUVAR_SP_TOP is incorrect");                               \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.mscratch0) == CPUVAR_MSCRATCH0, \
                  "CPUVAR_MSCRATCH0 is incorrect");                            \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.mscratch1) == CPUVAR_MSCRATCH1, \
                  "CPUVAR_MSCRATCH1 is incorrect");                            \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.mtimecmp) == CPUVAR_MTIMECMP,   \
                  "CPUVAR_MTIMECMP is incorrect");                             \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.mtime) == CPUVAR_MTIME,         \
                  "CPUVAR_MTIME is incorrect");                                \
    STATIC_ASSERT(offsetof(struct cpuvar, arch.interval) == CPUVAR_INTERVAL,   \
                  "CPUVAR_INTERVAL is incorrect");

// CPUVARマクロの中身。現在のCPUローカル変数のアドレスを返す。
static inline struct cpuvar *arch_cpuvar_get(void) {
    // tpレジスタにCPUローカル変数のアドレスが格納されている。
    uint32_t tp;
    __asm__ __volatile__("mv %0, tp" : "=r"(tp));
    return (struct cpuvar *) tp;
}

// 物理アドレスを仮想アドレスに変換する。
static inline vaddr_t arch_paddr_to_vaddr(paddr_t paddr) {
    // 0x80000000 以上の物理アドレスは同じ仮想アドレスにマッピングされているため、
    // そのまま返す。
    return paddr;
}

// ユーザータスクが利用してもよい仮想アドレスかどうかを返す。
static inline bool arch_is_mappable_uaddr(uaddr_t uaddr) {
    // 0番地付近はヌルポインタ参照の可能性が高いため許可しない。また、KERNEL_BASE以降は
    // カーネルが利用するので許可しない。
    return PAGE_SIZE <= uaddr && uaddr < KERNEL_BASE;
}
