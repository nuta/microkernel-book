#include "mp.h"
#include "asm.h"
#include <kernel/arch.h>
#include <kernel/printk.h>
#include <kernel/task.h>

static struct cpuvar cpuvars[NUM_CPUS_MAX];
static uint32_t big_lock = BKL_UNLOCKED;
static int locked_cpu = -1;

// setssipレジスタ (ACLINT) への書き込みをしてIPIを発行する。
static void write_setssip(uint32_t hartid) {
    full_memory_barrier();
    mmio_write32_paddr(ACLINT_SSWI_SETSSIP(hartid), 1);
}

// カーネルロック取得前に満たしておくべき条件をチェックする
static void check_lock(void) {
    DEBUG_ASSERT((read_sstatus() & SSTATUS_SIE) == 0);

    if (big_lock == BKL_HALTED) {
        // 他のCPUが停止状態に入っている。そのCPUがパニックメッセージを出力するために
        // 無理矢理カーネルロックを奪っているため、処理を進めるとまずい。
        for (;;) {
            asm_wfi();
        }
    }
}

// カーネルロックを取得する
void mp_lock(void) {
    check_lock();  // ロックの状態をチェック (デバッグ用)

    // 書き込み成功するまで試行し続ける
    while (true) {
        if (compare_and_swap(&big_lock, BKL_UNLOCKED, BKL_LOCKED)) {
            break;
        }
    }

    locked_cpu = CPUVAR->id;  // カーネルロックを持っているCPUのIDを記録

    // ここ以降のメモリ読み書きが上のロック取得前に行われないようにする (メモリバリア)。
    // これがないとCPUやコンパイラが並び替えてしまう恐れがある。
    full_memory_barrier();
}

// カーネルロックを解放する
void mp_unlock(void) {
    DEBUG_ASSERT(CPUVAR->id == locked_cpu);

    // ここ以前のメモリ読み書きが上のロック解放前に行われるようにする (メモリバリア)。
    // これがないとCPUやコンパイラが並び替えてしまう恐れがある。
    full_memory_barrier();

    // ロックを解放
    compare_and_swap(&big_lock, BKL_LOCKED, BKL_UNLOCKED);
}

// カーネルロックを強制的に取得する。カーネルパニックなど致命的なエラーが発生したときに
// 他のCPUからロックを奪い取ってカーネルを停止するために使う。
void mp_force_lock(void) {
    big_lock = BKL_LOCKED;
    locked_cpu = CPUVAR->id;
    full_memory_barrier();
}

// 指定したCPUのCPUローカル変数を取得する
struct cpuvar *riscv32_cpuvar_of(int hartid) {
    ASSERT(hartid < NUM_CPUS_MAX);
    return &cpuvars[hartid];
}

// 他のCPUにプロセッサ間割り込み (IPI) を送信する
void arch_send_ipi(unsigned ipi) {
    // 自身を除いた全CPUにIPIを送信する
    for (int hartid = 0; hartid < NUM_CPUS_MAX; hartid++) {
        struct cpuvar *cpuvar = riscv32_cpuvar_of(hartid);

        // 起動が完了しているCPUかつ自身以外かチェック
        if (cpuvar->online && hartid != CPUVAR->id) {
            // IPIの送信理由を宛先CPUのローカル変数に記録する (アトミックな |= 演算)
            atomic_fetch_and_or(&cpuvar->ipi_pending, ipi);

            // IPIを送信する
            write_setssip(hartid);
        }
    }

    // 各CPUがIPIを処理するまで待つ
    for (int hartid = 0; hartid < NUM_CPUS_MAX; hartid++) {
        struct cpuvar *cpuvar = riscv32_cpuvar_of(hartid);
        if (cpuvar->online && hartid != CPUVAR->id) {
            // 一旦カーネルロックを解放して他のCPUがカーネルに入れるようにする
            mp_unlock();

            // CPUがIPIを処理するまで待つ
            unsigned pending;
            do {
                pending = atomic_load(&cpuvar->ipi_pending);
            } while (pending != 0);

            // カーネルロックを再取得
            mp_lock();
        }
    }
}

// 各CPUの初期化処理
void riscv32_mp_init_percpu(void) {
    CPUVAR->online = true;
}

// コンピュータを停止する
__noreturn void halt(void) {
    big_lock = BKL_HALTED;
    full_memory_barrier();

    WARN("kernel halted (CPU #%d)", CPUVAR->id);
    for (;;) {
        asm_wfi();
    }
}
