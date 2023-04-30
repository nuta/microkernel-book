#include "debug.h"
#include <kernel/arch.h>
#include <kernel/riscv32/asm.h>
#include <libs/common/print.h>

STATIC_ASSERT(IS_ALIGNED(KERNEL_STACK_SIZE, 2),
              "KERNEL_STACK_SIZE must be aligned to 2");

// カーネルスタックの下端を返す。
static uint32_t *stack_bottom(void) {
    // 現在のスタックポインタの値を取得する
    uint32_t sp;
    __asm__ __volatile__("mv %0, sp" : "=r"(sp));

    // カーネルスタックは常に KERNEL_STACK_SIZE の倍数のアドレスに配置される。そのため、
    // 下位ビットを切り捨てることでカーネルスタックの下端を求めることができる。
    return (uint32_t *) ALIGN_DOWN(sp, KERNEL_STACK_SIZE);
}

// 現在のカーネルスタックの下端にカーネルスタックの有効性をチェックするための値を書き込む。
void stack_reset_current_canary(void) {
    stack_set_canary((uint32_t) stack_bottom());
}

// カーネルスタックの下端にカーネルスタックの有効性をチェックするための値を書き込む。
// オーバーフローが発生した場合にはこの値が書き換えられているはず。
void stack_set_canary(uint32_t sp_bottom) {
    *((uint32_t *) sp_bottom) = STACK_CANARY_VALUE;
}

// カーネルスタックが有効か (オーバーフロー、有効な値か) をチェックする。また、ついでに
// 割り込みハンドラが呼び出される際に満たしておくべき条件も確認する。割り込み周りは厄介な
// バグが生まれやすいので、無意味に見えるチェックでもバグの早期発見に効果的である。
void stack_check(void) {
    if (CPUVAR->magic != CPUVAR_MAGIC) {
        PANIC("invalid CPUVAR: addr=%p, magic=%x", CPUVAR, CPUVAR->magic);
    }

    if (!CPUVAR->online) {
        PANIC("CPUVAR->online is false (sepc=%p, stval=%p, scause=%x)",
              read_sepc(), read_stval(), read_scause());
    }

    if (*stack_bottom() != STACK_CANARY_VALUE) {
        PANIC("kernel stack has been exhausted (sepc=%p, stval=%p, scause=%x)",
              read_sepc(), read_stval(), read_scause());
    }
}
