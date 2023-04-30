#include "asm.h"
#include "debug.h"
#include "handler.h"
#include "mp.h"
#include "plic.h"
#include "trap.h"
#include "uart.h"
#include "vm.h"
#include <kernel/arch.h>
#include <kernel/main.h>
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <kernel/task.h>
#include <libs/common/string.h>

// 0番目のCPUが起動したことを示すフラグ
static volatile bool hart0_ready = false;

// 0番目のCPUのブート処理: riscv32_setup関数の末尾からジャンプされる。ここからS-Mode。
__noreturn void riscv32_setup(void) {
    // カーネルロックを取得する。ここから他のCPUと共有するデータを操作できる。
    mp_lock();

    // PLIC (割り込みコントローラ) を初期化する。
    riscv32_plic_init_percpu();

    // ここからkernel_main関数に渡すブート情報を構築する。
    struct bootinfo bootinfo;
    bootinfo.boot_elf = (paddr_t) __boot_elf;  // vmサーバのELFイメージ

    struct memory_map_entry *frees =
        (struct memory_map_entry *) &bootinfo.memory_map.frees;
    struct memory_map_entry *devices =
        (struct memory_map_entry *) &bootinfo.memory_map.devices;

    vaddr_t ram_start = (vaddr_t) __ram_start;            // RAM領域の開始アドレス
    vaddr_t free_ram_start = (vaddr_t) __free_ram_start;  // カーネルメモリ以後

    // ユーザーランドが使える空きメモリ領域
    frees[0].paddr = ALIGN_UP((paddr_t) __free_ram_start, PAGE_SIZE);
    frees[0].size = RAM_SIZE - (free_ram_start - ram_start);
    bootinfo.memory_map.num_frees = 1;

    // virtio-(blk|net) のMMIO領域
    devices[0].paddr = VIRTIO_BLK_PADDR;
    devices[0].size = 0x1000;
    devices[1].paddr = VIRTIO_NET_PADDR;
    devices[1].size = 0x1000;
    bootinfo.memory_map.num_devices = 2;

    DEBUG_ASSERT(bootinfo.memory_map.num_devices <= NUM_MEMORY_MAP_ENTRIES_MAX);

    // 現在使用中のスタックの底にスタックオーバーフローを検出するための値を設定する。
    stack_reset_current_canary();

    // カーネルのブート処理を続ける。
    kernel_main(&bootinfo);
    UNREACHABLE();
}

// 0番目以外のCPUのブート処理: riscv32_setup関数の末尾からジャンプされる。ここからS-Mode。
__noreturn void riscv32_setup_mp(void) {
    // カーネルロックを取得する。ここから他のCPUと共有するデータを操作できる。
    mp_lock();

    // PLIC (割り込みコントローラ) を初期化する。
    riscv32_plic_init_percpu();

    // 現在使用中のスタックの底にスタックオーバーフローを検出するための値を設定する。
    stack_reset_current_canary();

    // カーネルのブート処理を続ける。
    kernel_mp_main();
    UNREACHABLE();
}

// M-mode上で実行されるブート処理。M-modeでしか行えない初期化処理を行い、S-modeに移行する。
//
// ブート処理では、一度のみ行う初期化処理をすべて0番目のCPUで行う。その他のCPUは、0番目のCPUが
// 起動処理を終えるまで待機した後にブート処理を開始する。
//
// 注意: この関数ではCPUローカル変数以外のデータ領域にアクセスしてはならない。カーネルロックを
//       ここではまだ取得しないため、他のCPUと競合してデータを破壊してしまう。
__noreturn void riscv32_boot(void) {
    int hartid = read_mhartid();  // CPU番号
    if (hartid == 0) {
        // .bssセクションをゼロクリア。最初にやるべき。
        memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
        // UARTを初期化して <libs/common/print.h> で定義されたマクロを使えるようにする。
        riscv32_uart_init();
    } else {
        // 0番目のCPU以外は、0番目のCPUが初期化処理を終えるまで待機する。
        while (!hart0_ready)
            ;
    }

    // 全ての割り込み・例外をS-modeで処理するようにする。
    write_medeleg(0xffff);
    write_mideleg(0xffff);

    // 物理メモリのアクセス権限を設定する。
    write_pmpaddr0(0xffffffff);
    write_pmpcfg0(0xf);

    // CPUローカル変数を初期化する。
    struct cpuvar *cpuvar = riscv32_cpuvar_of(hartid);
    memset(cpuvar, 0, sizeof(struct cpuvar));
    cpuvar->magic = CPUVAR_MAGIC;
    cpuvar->online = false;  // まだブート中
    cpuvar->id = hartid;
    cpuvar->ipi_pending = 0;
    cpuvar->arch.interval = MTIME_PER_1MS;
    cpuvar->arch.mtimecmp = CLINT_MTIMECMP(hartid);
    cpuvar->arch.mtime = CLINT_MTIME;

    // CPUローカル変数を割り込みハンドラで使うレジスタ (sscratch, mscratch) とtpレジスタ
    // に設定する。ここからは、CPUローカル変数をCPUVARマクロでアクセスできるようになる。
    write_sscratch((uint32_t) cpuvar);
    write_mscratch((uint32_t) cpuvar);
    write_tp((uint32_t) cpuvar);

    // タイマーを設定するが、まだ割り込みが来て欲しくないので十分長い時間を設定する。
    *MTIMECMP = 0xffffffff;

    // S-modeの割り込みハンドラと、例外ハンドラで使われるカーネルスタックを設定する。
    //
    // ただし、ブート処理を完了して最初のユーザータスクを実行し始めるまでは例外・割り込み共に
    // 発生しないはず。とはいえ、バグで起きることは十分考えられるので、でたらめな値 (0xdeadbeef)
    // を設定して気づけるようにしておく。
    cpuvar->arch.sp_top = 0xdeadbeef;
    write_stvec((uint32_t) riscv32_trap_handler);

    // M-modeの割り込みハンドラと各種設定を行う。名前の通り、タイマー割り込みだけはM-modeの
    // 割り込みハンドラが呼ばれる。
    write_mtvec((uint32_t) riscv32_timer_handler);
    write_mstatus(read_mstatus() | MSTATUS_MIE);
    write_mie(read_mie() | MIE_MTIE);

    // mret命令で飛ぶ先のアドレスを設定する。
    if (hartid == 0) {
        write_mepc((uint32_t) riscv32_setup);
    } else {
        write_mepc((uint32_t) riscv32_setup_mp);
    }

    // mret命令でS-modeに戻るようにする。
    uint32_t mstatus = read_mstatus();
    mstatus &= ~MSTATUS_MPP_MASK;
    mstatus |= MSTATUS_MPP_S;
    write_mstatus(mstatus);

    // mret命令は本来割り込みハンドラから発生時の状態を復元して戻る命令だが、この仕組みを
    // 活用してここではmepc/mstatusレジスタに設定した「状態」に遷移させる。
    asm_mret();
    UNREACHABLE();
}

void arch_init(void) {
    riscv32_vm_init();
}

void arch_init_percpu(void) {
    // タスクが既に初期化されているはず
    ASSERT(CURRENT_TASK == IDLE_TASK);

    // 最初のコンテキストスイッチまでの間に割り込みが発生しないはずなので、ここで設定した値が
    // 使われることはない。とはいえカーネルのバグで例外が発生するかもしれないため念のため
    // ここで設定しておく。
    CPUVAR->arch.sp_top = IDLE_TASK->arch.sp_top;

    // ユーザーページにS-mode (カーネル) からアクセスできるようにする。
    write_sstatus(read_sstatus() | SSTATUS_SUM);

    // S-modeで受け付ける割り込みの種類 (sieレジスタ) を設定する。ただしsstatusレジスタでは
    // 割り込みを無効化したままなので、割り込みハンドラが呼ばれないはず。
    write_sie(read_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

    // CPUを起動済みとしてマークする。
    riscv32_mp_init_percpu();

    // タイマー割り込みを設定する。
    uint32_t last_mtime = *MTIME;
    *MTIMECMP = last_mtime + CPUVAR->arch.interval;
    CPUVAR->arch.last_mtime = last_mtime;

    if (CPUVAR->id == 0) {
        hart0_ready = true;
        arch_irq_enable(UART0_IRQ);
    }
}

// アイドルタスクのメイン処理。割り込みが来るまでCPUを休ませる。
void arch_idle(void) {
    // 割り込みハンドラが自身でカーネルロックをとっているので、ここではロックを解除する
    mp_unlock();

    // 割り込みを有効化して待つ。つまりCPUが割り込みが来るまでお休みする。
    write_sstatus(read_sstatus() | SSTATUS_SIE);
    asm_wfi();

    // 割り込みハンドラが処理を終えてこの関数に戻ってきた。
    // 割り込みを無効化してカーネルロックを取り直す。
    write_sstatus(read_sstatus() & ~SSTATUS_SIE);
    mp_lock();
}

__noreturn void arch_shutdown(void) {
    // ページングを無効化する
    write_satp(0);

    // SiFiveテストデバイスという架空のデバイスを呼び出してエミュレータを終了する。
    // ページングを無効化しているため、物理アドレス 0x100000 にアクセスできる。
    *((volatile uint32_t *) 0x100000) = 0x5555 /* 正常終了*/;

    PANIC("failed to shutdown");
}

// パニック関数が呼ばれ、パニックメッセージを出力する前に呼ばれる。
void panic_before_hook(void) {
    // 無理矢理カーネルロックを取得してパニックメッセージを出力する。
    mp_force_lock();
}

// パニック関数が呼ばれ、パニックメッセージを出力した後に呼ばれる。
__noreturn void panic_after_hook(void) {
    // コンピュータを停止する。ただしトラブルシューティングできるようシャットダウンはしない。
    halt();
}
