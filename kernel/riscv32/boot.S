#include <buildconfig.h>
.section ".boot", "ax"

// カーネルのエントリーポイント。ブートローダーからジャンプされ、ここから実行が開始される。
.global boot
boot:
    // スタックトレースがここで止まるようにするため、raとfpをゼロクリアする。
    mv ra, zero
    mv fp, zero

    // スタックポインタを設定する。CPUごとに独自のブートスタックを用意している。
    //
    // 開始アドレス: __boot_stack + KERNEL_STACK_SIZE * CPU番号
    // 終了アドレス: __boot_stack + KERNEL_STACK_SIZE * (CPU番号 + 1)
    //
    // スタックは下向きに伸びていくので、終了アドレスをスタックポインタに設定する。
    csrr a0, mhartid
    li a1, KERNEL_STACK_SIZE
    addi a0, a0, 1            // a0 = CPU番号 + 1
    mul a0, a0, a1            // a0 = (CPU番号 + 1) * KERNEL_STACK_SIZE
    la sp, __boot_stack
    add sp, sp, a0            // sp = __boot_stack + a0

    // C言語のプログラムを実行する準備 (スタック) ができたので、早速処理を移行する。
    jal riscv32_boot
