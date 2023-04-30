#include "syscall.h"
#include "arch.h"
#include "hinavm.h"
#include "interrupt.h"
#include "ipc.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include <libs/common/string.h>

// ユーザー空間からのメモリコピー。通常のmemcpyと異なり、コピー中にページフォルトが発生した場合
// には、カーネルランドではなくユーザータスクで発生したページフォルトとして処理する。
error_t memcpy_from_user(void *dst, __user const void *src, size_t len) {
    if (!arch_is_mappable_uaddr((uaddr_t) src)) {
        // カーネルモードでは全ての仮想アドレスにアクセスできる権限があるので、渡されたポインタが
        // 確かにユーザー空間のアドレスを指していることを確認する。
        return ERR_INVALID_UADDR;
    }

    arch_memcpy_from_user(dst, src, len);
    return OK;
}

// ユーザー空間へのメモリコピー。通常のmemcpyと異なり、コピー中にページフォルトが発生した場合
// には、カーネルではなく実行中タスクで発生したページフォルトとして処理する。
error_t memcpy_to_user(__user void *dst, const void *src, size_t len) {
    if (!arch_is_mappable_uaddr((uaddr_t) dst)) {
        // カーネルモードでは全ての仮想アドレスにアクセスできる権限があるので、渡されたポインタが
        // 確かにユーザー空間のアドレスを指していることを確認する。
        return ERR_INVALID_UADDR;
    }

    arch_memcpy_to_user(dst, src, len);
    return OK;
}

// dst_lenバイトをユーザー空間からコピーする。コピー後、ヌル終端されていなければエラーを返す。
static error_t strcpy_from_user(char *dst, size_t dst_len,
                                __user const char *src) {
    DEBUG_ASSERT(dst_len > 0);

    error_t err = memcpy_from_user(dst, src, dst_len);
    if (err != OK) {
        return err;
    }

    // ヌル終端されているかチェック。
    for (size_t i = 0; i < dst_len; i++) {
        if (dst[i] == '\0') {
            return OK;
        }
    }

    return ERR_INVALID_ARG;
}

// 新規ユーザータスクを作成する。
static task_t sys_task_create(__user const char *name, uaddr_t ip,
                              task_t pager) {
    // タスク名を取得
    char namebuf[TASK_NAME_LEN];
    error_t err = strcpy_from_user(namebuf, sizeof(namebuf), name);
    if (err != OK) {
        return err;
    }

    // ページャータスクを取得
    struct task *pager_task = task_find(pager);
    if (!pager_task) {
        return ERR_INVALID_ARG;
    }

    // 通常のユーザータスクを作成する場合
    return task_create(namebuf, ip, pager_task);
}

// HinaVMタスクを生成する。instsはHinaVM命令列、num_instsは命令数、pagerはページャータスク。
static task_t sys_hinavm(__user const char *name, __user hinavm_inst_t *insts,
                         size_t num_insts, task_t pager) {
    // タスク名を取得
    char namebuf[TASK_NAME_LEN];
    error_t err = strcpy_from_user(namebuf, sizeof(namebuf), name);
    if (err != OK) {
        return err;
    }

    // ページャータスクを取得
    struct task *pager_task = task_find(pager);
    if (!pager_task) {
        return ERR_INVALID_ARG;
    }

    hinavm_inst_t instsbuf[HINAVM_INSTS_MAX];
    if (num_insts > HINAVM_INSTS_MAX) {
        WARN("too many instructions: %u (max=%u)", num_insts, HINAVM_INSTS_MAX);
        return ERR_INVALID_ARG;
    }

    // HinaVMのプログラム (命令列) を取得
    err = memcpy_from_user(instsbuf, insts, num_insts * sizeof(*insts));
    if (err != OK) {
        return err;
    }

    // HinaVMタスクを作成
    return hinavm_create(namebuf, instsbuf, num_insts, pager_task);
}

// タスクを削除する。
static error_t sys_task_destroy(task_t tid) {
    struct task *task = task_find(tid);
    if (!task || task == CURRENT_TASK) {
        return ERR_INVALID_TASK;
    }

    return task_destroy(task);
}

// 実行中タスクを正常終了する。
__noreturn static void sys_task_exit(void) {
    task_exit(EXP_GRACE_EXIT);
}

// 実行中タスクのタスクIDを取得する。
static task_t sys_task_self(void) {
    return CURRENT_TASK->tid;
}

// 物理ページを割り当てる。
//
// flagsにPM_ALLOC_ALIGNが指定されている場合は、sizeバイトにアラインされたアドレスを
// 割り当てる。
static pfn_t sys_pm_alloc(task_t tid, size_t size, unsigned flags) {
    // 未知・許可されていないフラグが指定されていないかチェック
    if ((flags & ~(PM_ALLOC_ZEROED | PM_ALLOC_ALIGNED)) != 0) {
        return ERR_INVALID_ARG;
    }

    // 所有者となるタスクを取得
    struct task *task = task_find(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    if (task != CURRENT_TASK && task->pager != CURRENT_TASK) {
        return ERR_INVALID_TASK;
    }

    flags |= PM_ALLOC_ZEROED;
    paddr_t paddr = pm_alloc(size, task, flags);
    if (!paddr) {
        return ERR_NO_MEMORY;
    }

    return PADDR2PFN(paddr);
}

// ページを仮想アドレス空間にマップする。
static paddr_t sys_vm_map(task_t tid, uaddr_t uaddr, paddr_t paddr,
                          unsigned attrs) {
    // 操作対象のタスクを取得
    struct task *task = task_find(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    // 未知・許可されていないフラグが指定されていないかチェック
    if ((attrs & ~(PAGE_WRITABLE | PAGE_READABLE | PAGE_EXECUTABLE)) != 0) {
        return ERR_INVALID_ARG;
    }

    // ページ境界にアラインされているかチェック
    if (!IS_ALIGNED(uaddr, PAGE_SIZE) || !IS_ALIGNED(paddr, PAGE_SIZE)) {
        return ERR_INVALID_ARG;
    }

    // 仮想アドレスがマップ可能かチェック
    if (!arch_is_mappable_uaddr(uaddr)) {
        return ERR_INVALID_UADDR;
    }

    attrs |= PAGE_USER;  // 常にユーザーページとしてマップする
    return vm_map(task, uaddr, paddr, attrs);
}

// ページを仮想アドレス空間からアンマップする。
static paddr_t sys_vm_unmap(task_t tid, uaddr_t uaddr) {
    // 操作対象のタスクを取得
    struct task *task = task_find(tid);
    if (!task) {
        return ERR_INVALID_TASK;
    }

    // ページ境界にアラインされているかチェック
    if (!IS_ALIGNED(uaddr, PAGE_SIZE)) {
        return ERR_INVALID_ARG;
    }

    // 仮想アドレスがアンマップ可能かチェック
    if (!arch_is_mappable_uaddr(uaddr)) {
        return ERR_INVALID_UADDR;
    }

    return vm_unmap(task, uaddr);
}

// メッセージを送受信する。
static error_t sys_ipc(task_t dst, task_t src, __user struct message *m,
                       unsigned flags) {
    // 許可されていないフラグが指定されていないかチェック
    if ((flags & ~(IPC_SEND | IPC_RECV | IPC_NOBLOCK)) != 0) {
        return ERR_INVALID_ARG;
    }

    // 有効なタスクIDかチェック
    if (src < 0 || src > NUM_TASKS_MAX) {
        return ERR_INVALID_ARG;
    }

    // 送信処理が含まれているのであれば、送信先タスクを取得
    struct task *dst_task = NULL;
    if (flags & IPC_SEND) {
        dst_task = task_find(dst);
        if (!dst_task) {
            return ERR_INVALID_TASK;
        }
    }

    return ipc(dst_task, src, m, flags);
}

// 通知を送信する。
static error_t sys_notify(task_t dst, notifications_t notifications) {
    struct task *dst_task = task_find(dst);
    if (!dst_task) {
        return ERR_INVALID_TASK;
    }

    notify(dst_task, notifications);
    return OK;
}

// 割り込み通知を購読する。
static error_t sys_irq_listen(unsigned irq) {
    return irq_listen(CURRENT_TASK, irq);
}

// 割り込み通知の購読を解除する。
static error_t sys_irq_unlisten(unsigned irq) {
    return irq_unlisten(CURRENT_TASK, irq);
}

// シリアルポートへの書き込みを行う。
static int sys_serial_write(__user const char *buf, size_t buf_len) {
    // シリアルポートへの書き込みは時間がかかるため、最大文字数を制限する。
    int written_len = MIN(buf_len, 4096);

    char kbuf[512];
    int remaining = written_len;
    while (remaining > 0) {
        // 書き込む文字列を一時バッファにコピーする。
        int copy_len = MIN(remaining, (int) sizeof(kbuf));
        memcpy_from_user(kbuf, buf, copy_len);

        // 一時バッファの内容をシリアルポートに書き込む。
        for (int i = 0; i < copy_len; i++) {
            arch_serial_write(kbuf[i]);
        }

        remaining -= copy_len;
    }

    return written_len;
}

// シリアルポートからの読み込みを行う。
static int sys_serial_read(__user char *buf, int max_len) {
    // シリアルポートの受信済み文字列を一時バッファにコピーする。
    char tmp[128];
    int len = serial_read((char *) &tmp, MIN(max_len, (int) sizeof(tmp)));

    // 一時バッファの内容をユーザーバッファにコピーする。
    memcpy_to_user(buf, tmp, len);
    return len;
}

// タイムアウトを設定する。呼び出した時点から指定した時間 (ミリ秒) が経過すると、タスクに通知が
// 送られる。値がゼロの場合は、タイムアウトを解除する。
static error_t sys_time(int timeout) {
    if (timeout < 0) {
        return ERR_INVALID_ARG;
    }

    // タイムアウト時間を更新する
    CURRENT_TASK->timeout = (timeout == 0) ? 0 : (timeout * (TICK_HZ / 1000));
    return OK;
}

// 起動してからの経過時間をミリ秒単位で返す。
static int sys_uptime(void) {
    return uptime_ticks / TICK_HZ;
}

// コンピューターの電源を切る。
__noreturn static int sys_shutdown(void) {
    arch_shutdown();
}

// システムコールハンドラ
long handle_syscall(long a0, long a1, long a2, long a3, long a4, long n) {
    long ret;
    switch (n) {
        case SYS_IPC:
            ret = sys_ipc(a0, a1, (__user struct message *) a2, a3);
            break;
        case SYS_NOTIFY:
            ret = sys_notify(a0, a1);
            break;
        case SYS_SERIAL_WRITE:
            ret = sys_serial_write((__user const char *) a0, a1);
            break;
        case SYS_SERIAL_READ:
            ret = sys_serial_read((__user char *) a0, a1);
            break;
        case SYS_TASK_CREATE:
            ret = sys_task_create((__user const char *) a0, a1, a2);
            break;
        case SYS_TASK_DESTROY:
            ret = sys_task_destroy(a0);
            break;
        case SYS_TASK_EXIT:
            sys_task_exit();
            UNREACHABLE();
        case SYS_TASK_SELF:
            ret = sys_task_self();
            break;
        case SYS_PM_ALLOC:
            ret = sys_pm_alloc(a0, a1, a2);
            break;
        case SYS_VM_MAP:
            ret = sys_vm_map(a0, a1, a2, a3);
            break;
        case SYS_VM_UNMAP:
            ret = sys_vm_unmap(a0, a1);
            break;
        case SYS_IRQ_LISTEN:
            ret = sys_irq_listen(a0);
            break;
        case SYS_IRQ_UNLISTEN:
            ret = sys_irq_unlisten(a0);
            break;
        case SYS_HINAVM:
            ret = sys_hinavm((__user const char *) a0,
                             (__user hinavm_inst_t *) a1, a2, a3);
            break;
        case SYS_TIME:
            ret = sys_time(a0);
            break;
        case SYS_UPTIME:
            ret = sys_uptime();
            break;
        case SYS_SHUTDOWN:
            ret = sys_shutdown();
            break;
        default:
            ret = ERR_INVALID_ARG;
    }

    return ret;
}
