#include "trap.h"
#include "asm.h"
#include "debug.h"
#include "mp.h"
#include "plic.h"
#include "uart.h"
#include "usercopy.h"
#include "vm.h"
#include <kernel/interrupt.h>
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <kernel/syscall.h>
#include <kernel/task.h>

// システムコール
static void handle_syscall_trap(struct riscv32_trap_frame *frame) {
    // システムコールハンドラを呼び出し、戻り値をa0レジスタに設定
    frame->a0 = handle_syscall(frame->a0, frame->a1, frame->a2, frame->a3,
                               frame->a4, frame->a5);

    // 復元するプログラムカウンタの値を更新して、システムコールを呼び出す命令 (ecall命令) の
    // 次の命令に戻るようにする
    frame->pc += 4;
}

// ソフトウェア割り込み: プロセス間割り込みと、riscv32_timer_handler からのタイマ割り込み。
static void handle_soft_interrupt_trap(void) {
    write_sip(read_sip() & ~SIP_SSIP);  // SSIPビットをクリア

    // 処理すべきIPIがなくなるまで処理
    while (true) {
        // 処理すべきIPIを取得する。ipi_pending = ipi_pending & 0 と同じ。
        // つまり、ipi_pendingの値を取得してipi_pendingを0にする
        unsigned pending = atomic_fetch_and_and(&CPUVAR->ipi_pending, 0);
        if (pending == 0) {
            break;
        }

        // TLB shootdown
        if (pending & IPI_TLB_FLUSH) {
            asm_sfence_vma();
        }

        if (pending & IPI_RESCHEDULE) {
            task_switch();
        }
    }

    // タイマーが進んでいたら、タイマー割り込みハンドラを呼び出す
    unsigned now = *MTIME;
    unsigned ticks = MTIME_TO_TICKS(now - CPUVAR->arch.last_mtime);
    CPUVAR->arch.last_mtime = now;
    if (ticks > 0) {
        handle_timer_interrupt(ticks);
    }
}

// ハードウェア割り込み
static void handle_external_interrupt_trap(void) {
    // どの割り込みが発生したかを取得
    unsigned irq = riscv32_plic_pending();
    riscv32_plic_ack(irq);

    if (irq == UART0_IRQ) {
        // シリアルポートからの割り込み (文字の受信)
        handle_serial_interrupt();
    } else {
        // その他の割り込み
        handle_interrupt(irq);
    }
}

// ページフォルト
static void handle_page_fault_trap(struct riscv32_trap_frame *frame) {
    // 発生原因を取得
    uint32_t reason;
    switch (read_scause()) {
        case SCAUSE_INST_PAGE_FAULT:  // 命令の実行時にページフォルトが発生
            reason = PAGE_FAULT_EXEC;
            break;
        case SCAUSE_LOAD_PAGE_FAULT:  // メモリ読み込み時にページフォルトが発生
            reason = PAGE_FAULT_READ;
            break;
        case SCAUSE_STORE_PAGE_FAULT:  // メモリ書き込み時にページフォルトが発生
            reason = PAGE_FAULT_WRITE;
            break;
        default:
            UNREACHABLE();
    }

    // ページフォルトの原因となる仮想アドレスを取得
    vaddr_t vaddr = read_stval();

    // 既にマップされているページかチェックする。既にマップされている場合は、読み込み専用ページ
    // に書き込もうとする等のページ権限の違反。マップされていない場合は、デマンドページングか
    // NULLポインタ参照が考えられる。
    reason |= riscv32_is_mapped(read_satp(), vaddr) ? PAGE_FAULT_PRESENT : 0;

    if ((frame->sstatus & SSTATUS_SPP) == 0) {
        // ユーザーモードで発生した
        reason |= PAGE_FAULT_USER;
    }

    // 発生時のプログラムカウンタを取得
    uint32_t sepc = read_sepc();

    if (sepc == (uint32_t) riscv32_usercopy1
        || sepc == (uint32_t) riscv32_usercopy2) {
        // ユーザーポインタ上でのコピー中にページフォルトが発生した場合は、
        // ユーザーモードで発生したもの (PAGE_FAULT_USER) として処理する。
        //
        // このケースではカーネルロックを既に持っているため、ロックをここで取得する
        // (mp_lock関数を呼び出す) 必要はない。
        reason |= PAGE_FAULT_USER;
        handle_page_fault(vaddr, sepc, reason);
    } else {
        // カーネルモードでのページフォルトは致命的なエラーとして扱う。
        if ((reason & PAGE_FAULT_USER) == 0) {
            PANIC("page fault in kernel: vaddr=%p, sepc=%p, reason=%x", vaddr,
                  sepc, reason);
        }

        // ユーザーモードでのページフォルトはページャタスクを呼び出す。ページャタスクがマップ
        // するまでブロックするので注意。
        mp_lock();
        handle_page_fault(vaddr, sepc, reason);
        mp_unlock();
    }
}

// 割り込み・例外ハンドラ: ブート処理を完了したあとは、この関数がカーネルモードへの入り口となる。
void riscv32_handle_trap(struct riscv32_trap_frame *frame) {
    stack_check();  // スタックオーバーフローをチェック

    //
    //  注意: カーネルロックの取得・解放 (mp_lock/mp_unlock 関数) を忘れないこと
    //

    uint32_t scause = read_scause();  // 割り込みの原因を取得
    switch (scause) {
        // システムコール
        case SCAUSE_ENV_CALL:
            mp_lock();
            handle_syscall_trap(frame);
            mp_unlock();
            break;
        // ソフトウェア割り込み
        case SCAUSE_S_SOFT_INTR:
            mp_lock();
            handle_soft_interrupt_trap();
            mp_unlock();
            break;
        // 外部割り込み
        case SCAUSE_S_EXT_INTR:
            mp_lock();
            handle_external_interrupt_trap();
            mp_unlock();
            break;
        // ページフォルト
        case SCAUSE_INST_PAGE_FAULT:
        case SCAUSE_LOAD_PAGE_FAULT:
        case SCAUSE_STORE_PAGE_FAULT:
            handle_page_fault_trap(frame);
            break;
        // 実行中タスクが原因の例外
        case SCAUSE_INS_MISS_ALIGN:
        case SCAUSE_INST_ACCESS_FAULT:
        case SCAUSE_ILLEGAL_INST:
        case SCAUSE_BREAKPOINT:
        case SCAUSE_LOAD_ACCESS_FAULT:
        case SCAUSE_AMO_MISS_ALIGN:
        case SCAUSE_STORE_ACCESS_FAULT:
            WARN("%s: invalid exception: scause=%d, stval=%p",
                 CURRENT_TASK->name, read_scause(), read_stval());
            mp_lock();
            task_exit(EXP_ILLEGAL_EXCEPTION);
        default:
            PANIC("unknown trap: scause=%p, stval=%p", read_scause(),
                  read_stval());
    }

    if (frame->sstatus & SSTATUS_SPP) {
        // カーネルモードで発生した例外 (いわゆる、ネストされた例外) の場合、frame->tpに
        // 保存されている例外発生時のCPUのCPUVARと、例外処理を続行している現在のCPUのそれが
        // 異なるケースがある。たとえば、次のようなケースが考えられる:
        //
        // CPU 0: 例外処理中に他の例外 (例: ユーザーコピー中のページフォルト) が発生する。
        // CPU 0: ページフォルト処理要求をページャタスクに送信し、他のタスクの処理を進める。
        // CPU 1: ページャタスクからの返信が来たので、そのページフォルトで停止していたタスク
        //        を再開する。
        // CPU 1: ページフォルトハンドラから戻って frame の内容を復元し、元の例外処理
        //        (例: システムコールハンドラ) を続行する。ただし、frame->tpはCPU 0の
        //        CPUVARを指しているため、そのままではCPU 1がCPU 0のCPUローカル変数を
        //        参照してしまう。
        //
        // そのため、復帰するtpレジスタの内容を実行中CPUのそれに書き換えて辻褄を合わせる。
        frame->tp = (uint32_t) CPUVAR;
    }

    stack_check();  // スタックオーバーフローをチェック
}
