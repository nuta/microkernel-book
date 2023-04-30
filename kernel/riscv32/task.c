#include "asm.h"
#include "debug.h"
#include "mp.h"
#include "switch.h"
#include <kernel/arch.h>
#include <kernel/hinavm.h>
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <kernel/task.h>

// ユーザータスクへの最初のコンテキストスイッチ時に呼び出される関数
__noreturn void riscv32_user_entry(uint32_t ip) {
    mp_unlock();     // ユーザーモードへ入るのでカーネルロックを解放する
    write_sepc(ip);  // ユーザータスクの実行開始アドレスを設定

    // sret命令が復元すべき状態を設定する
    uint32_t sstatus = read_sstatus();
    sstatus &= ~SSTATUS_SPP;  //
    sstatus |= SSTATUS_SPIE;
    write_sstatus(sstatus);

    // カーネルの情報を漏らさないようにレジスタをクリアしてからユーザーモードに移行する
    __asm__ __volatile__(
        "mv x1, zero\n\t"
        "mv x2, zero\n\t"
        "mv x3, zero\n\t"
        "mv x4, zero\n\t"
        "mv x5, zero\n\t"
        "mv x6, zero\n\t"
        "mv x7, zero\n\t"
        "mv x8, zero\n\t"
        "mv x10, zero\n\t"
        "mv x11, zero\n\t"
        "mv x12, zero\n\t"
        "mv x13, zero\n\t"
        "mv x14, zero\n\t"
        "mv x15, zero\n\t"
        "mv x16, zero\n\t"
        "mv x17, zero\n\t"
        "mv x18, zero\n\t"
        "mv x19, zero\n\t"
        "mv x20, zero\n\t"
        "mv x21, zero\n\t"
        "mv x22, zero\n\t"
        "mv x23, zero\n\t"
        "mv x24, zero\n\t"
        "mv x25, zero\n\t"
        "mv x26, zero\n\t"
        "mv x27, zero\n\t"
        "mv x28, zero\n\t"
        "mv x29, zero\n\t"
        "mv x30, zero\n\t"
        "mv x31, zero\n\t"
        "sret");

    // この関数には決して戻ってこない。ユーザータスクからカーネルモードに戻るときは常に
    // riscv32_trap_handler が入り口となる。
    UNREACHABLE();
}

// 次のタスク (next) に切り替える。prevには現在実行中のタスクを指定する。
void arch_task_switch(struct task *prev, struct task *next) {
    // 割り込みハンドラで使うカーネルスタックを切り替える。システムコールハンドラ内で
    // スリープ状態に入ることがあるため、タスクごとに実行コンテキストを保持する専用の
    // カーネルスタックが必要。
    CPUVAR->arch.sp_top = next->arch.sp_top;

    // ページテーブルを切り替えてTLBをフラッシュする。satpレジスタに書き込む前に一度
    // sfence.vma命令を実行しているのは、ここ以前に行ったページテーブルへの変更が
    // 完了するのを保証するため。
    // (The RISC-V Instruction Set Manual Volume II, Version 1.10, p. 58)
    asm_sfence_vma();
    write_satp(SATP_MODE_SV32 | next->vm.table >> SATP_PPN_SHIFT);
    asm_sfence_vma();

    // レジスタを切り替えて次のタスク (next) に実行を移す。このタスク (prev) は
    // 実行コンテキストが保存され、再度続行されるときはこの関数から帰ってきたように
    // 振る舞う。
    riscv32_task_switch(&prev->arch.sp, &next->arch.sp);
}

// タスクを初期化する
error_t arch_task_init(struct task *task, uaddr_t ip, vaddr_t kernel_entry,
                       void *arg) {
    // カーネルスタックを割り当てる。スタックカナリー (stack canary) のアドレスを常に
    // stack_bottom 関数で計算できるように、 PM_ALLOC_ALIGNED フラグを指定して
    // スタックサイズの倍数のアドレスになるように割り当てる。
    paddr_t sp_bottom = pm_alloc(KERNEL_STACK_SIZE, NULL,
                                 PM_ALLOC_ALIGNED | PM_ALLOC_UNINITIALIZED);
    if (!sp_bottom) {
        return ERR_NO_MEMORY;
    }

    // カーネルスタックを用意する。スタックはアドレスの下方向に伸びることに注意。
    uint32_t sp_top = sp_bottom + KERNEL_STACK_SIZE;
    uint32_t *sp = (uint32_t *) arch_paddr_to_vaddr(sp_top);

    uint32_t entry;
    if (kernel_entry) {
        // riscv32_kernel_entry_trampoline関数でポップされる値
        *--sp = (uint32_t) kernel_entry;  // タスクの実行開始アドレス
        *--sp = (uint32_t) arg;           // a0レジスタ (第1引数) に渡される値
        entry = (uint32_t) riscv32_kernel_entry_trampoline;
    } else {
        // riscv32_user_entry_trampoline関数でポップされる値
        *--sp = ip;  // タスクの実行開始アドレス
        entry = (uint32_t) riscv32_user_entry_trampoline;
    }

    // riscv32_task_switch関数で復元される実行コンテキスト
    *--sp = 0;      // s11
    *--sp = 0;      // s10
    *--sp = 0;      // s9
    *--sp = 0;      // s8
    *--sp = 0;      // s7
    *--sp = 0;      // s6
    *--sp = 0;      // s5
    *--sp = 0;      // s4
    *--sp = 0;      // s3
    *--sp = 0;      // s2
    *--sp = 0;      // s1
    *--sp = 0;      // s0
    *--sp = entry;  // ra

    // タスク管理構造体を埋める
    task->arch.sp = (uint32_t) sp;
    task->arch.sp_bottom = sp_bottom;
    task->arch.sp_top = (uint32_t) sp_top;

    // スタックカナリーをカーネルスタックに設定する
    stack_set_canary(sp_bottom);
    return OK;
}

// タスクを破棄する
void arch_task_destroy(struct task *task) {
    pm_free(task->arch.sp_bottom, KERNEL_STACK_SIZE);
}
