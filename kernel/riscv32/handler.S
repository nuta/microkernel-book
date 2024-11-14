#include "asmdefs.h"

// 割り込みハンドラ。割り込みは自動的に無効化され、S-modeで実行される。
.align 4
.global riscv32_trap_handler
riscv32_trap_handler:
    csrrw tp, sscratch, tp     // tpレジスタの値をsscratchに退避し、そこに入っていた
                               // cpuvarのアドレスをtpに設定。
    sw a0, CPUVAR_SSCRATCH(tp) // a0レジスタの値を退避。


    // カーネルモードで割り込みが発生した際にはカーネルスタックをそのまま利用する。
    csrr a0, sstatus
    andi a0, a0, (1 << 8)      // SPPビット: 1ならカーネルモードで発生。
    bnez a0, 1f

    lw a0, CPUVAR_SP_TOP(tp)   // a0レジスタに実行中タスクのカーネルスタックを設定。
    j 2f
1:
    mv a0, sp                  // 現在のカーネルスタックをそのまま利用する。
2:
    // カーネルが利用する (つまり破壊してしまう) 汎用レジスタをカーネルスタックに退避し、
    // 後ほど復元できるようにする。各レジスタの保存先はriscv32_trap_frame構造体に従う。
    //
    // 色んなところでオフセット値に4をかけているのは、32ビットレジスタのサイズが
    // 4バイトだから。

    addi a0, a0, -4 * 33       // スタックから32ビットレジスタ33個分の領域を確保。
    sw ra,  4 * 2(a0)
    sw sp,  4 * 3(a0)
    sw gp,  4 * 4(a0)
    sw tp,  4 * 5(a0)
    sw t0,  4 * 6(a0)
    sw t1,  4 * 7(a0)
    sw t2,  4 * 8(a0)
    sw t3,  4 * 9(a0)
    sw t4,  4 * 10(a0)
    sw t5,  4 * 11(a0)
    sw t6,  4 * 12(a0)
    sw a1,  4 * 14(a0)
    sw a2,  4 * 15(a0)
    sw a3,  4 * 16(a0)
    sw a4,  4 * 17(a0)
    sw a5,  4 * 18(a0)
    sw a6,  4 * 19(a0)
    sw a7,  4 * 20(a0)
    sw s0,  4 * 21(a0)
    sw s1,  4 * 22(a0)
    sw s2,  4 * 23(a0)
    sw s3,  4 * 24(a0)
    sw s4,  4 * 25(a0)
    sw s5,  4 * 26(a0)
    sw s6,  4 * 27(a0)
    sw s7,  4 * 28(a0)
    sw s8,  4 * 29(a0)
    sw s9,  4 * 30(a0)
    sw s10, 4 * 31(a0)
    sw s11, 4 * 32(a0)

    mv sp, a0               // spレジスタを退避したので、spレジスタにカーネルスタックを
                            // 設定する。

    csrrw a0, sscratch, tp  // sscratchにcpuvarのアドレスを再度設定する。
    sw a0, 4 * 5(sp)        // sscratchから取り出した実行中タスクのtpレジスタを退避。

    lw a0, CPUVAR_SSCRATCH(tp) // 実行中タスクのa0レジスタの値を復元して、カーネルスタック
    sw a0, 4 * 13(sp)          // に退避。

    csrr a0, sepc
    sw a0, 4 * 0(sp) // どこで割り込みが発生したか (sepcレジスタ) を退避。
    csrr a0, sstatus
    sw a0, 4 * 1(sp) // 割り込み発生時のsstatusレジスタを退避。

    // 実行コンテキストの保存が完了したので、spレジスタを引数にしてCで書かれた
    // 割り込みハンドラに処理を移す。
    mv a0, sp
    call riscv32_handle_trap

    // 割り込みハンドラから戻ってきた。実行コンテキストをカーネルスタックから復元して
    // 処理を再開する。

    // sepcとsstatusを復元する。
    lw a0, 4 * 0(sp)
    csrw sepc, a0
    lw a0, 4 * 1(sp)
    csrw sstatus, a0

    // 各汎用レジスタを復元する。
    lw ra,  4 * 2(sp)
    lw gp,  4 * 4(sp)
    lw tp,  4 * 5(sp)
    lw t0,  4 * 6(sp)
    lw t1,  4 * 7(sp)
    lw t2,  4 * 8(sp)
    lw t3,  4 * 9(sp)
    lw t4,  4 * 10(sp)
    lw t5,  4 * 11(sp)
    lw t6,  4 * 12(sp)
    lw a0,  4 * 13(sp)
    lw a1,  4 * 14(sp)
    lw a2,  4 * 15(sp)
    lw a3,  4 * 16(sp)
    lw a4,  4 * 17(sp)
    lw a5,  4 * 18(sp)
    lw a6,  4 * 19(sp)
    lw a7,  4 * 20(sp)
    lw s0,  4 * 21(sp)
    lw s1,  4 * 22(sp)
    lw s2,  4 * 23(sp)
    lw s3,  4 * 24(sp)
    lw s4,  4 * 25(sp)
    lw s5,  4 * 26(sp)
    lw s6,  4 * 27(sp)
    lw s7,  4 * 28(sp)
    lw s8,  4 * 29(sp)
    lw s9,  4 * 30(sp)
    lw s10, 4 * 31(sp)
    lw s11, 4 * 32(sp)
    lw sp,  4 * 3(sp)
    sret                // 割り込みハンドラから戻る

// タイマー割り込みハンドラ。M-modeで実行される。
.align 4
.global riscv32_timer_handler
riscv32_timer_handler:
    csrrw a0, mscratch, a0       // mscratchからstruct cpuvarのアドレスを取り出す。

    sw a1, CPUVAR_MSCRATCH0(a0)  // 一時保存領域にa1レジスタを退避
    sw a2, CPUVAR_MSCRATCH1(a0)  // 一時保存領域にa2レジスタを退避

    lw a1, CPUVAR_MTIME(a0)      // mtimeレジスタのアドレスを取得
    lw a1, (a1)                  // mtimeレジスタの値を取得
    lw a2, CPUVAR_INTERVAL(a0)   // タイマー割り込み間隔を取得
    add a1, a1, a2               // 次のタイマー割り込み時刻を計算

    lw a2, CPUVAR_MTIMECMP(a0)   // mtimecmpレジスタのアドレスを取得
    sw a1, (a2)                  // mtimecmpレジスタに次のタイマー割り込み時刻を設定

    li a2, (1 << 1)              // SSIPビットをクリアするための値を設定
    csrw sip, a2                 // SSIPビットをクリア: S-modeでソフトウェア割り込みを起こす

    lw a1, CPUVAR_MSCRATCH0(a0)  // 一時保存領域からa1レジスタを復元
    lw a2, CPUVAR_MSCRATCH1(a0)  // 一時保存領域からa2レジスタを復元
    csrrw a0, mscratch, a0       // mscratchにcpuvarのアドレスを再度設定し、a0レジスタを復元
    mret                         // 割り込みハンドラから戻る
