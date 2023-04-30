#include <libs/common/hinavm_types.h>
#include <libs/common/print.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>

// ipcシステムコール: メッセージの送受信
error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags) {
    return arch_syscall(dst, src, (uintptr_t) m, flags, 0, SYS_IPC);
}

// notifyシステムコール: 通知の送信
error_t sys_notify(task_t dst, notifications_t notifications) {
    return arch_syscall(dst, notifications, 0, 0, 0, SYS_NOTIFY);
}

// task_createシステムコール: タスクの生成
task_t sys_task_create(const char *name, vaddr_t ip, task_t pager) {
    return arch_syscall((uintptr_t) name, ip, pager, 0, 0, SYS_TASK_CREATE);
}

// hinavmシステムコール: HinaVMプログラムの実行
task_t sys_hinavm(const char *name, hinavm_inst_t *insts, size_t num_insts,
                  task_t pager) {
    return arch_syscall((uintptr_t) name, (uintptr_t) insts, num_insts, pager,
                        0, SYS_HINAVM);
}

// task_destroyシステムコール: タスクの削除
error_t sys_task_destroy(task_t task) {
    return arch_syscall(task, 0, 0, 0, 0, SYS_TASK_DESTROY);
}

// task_exitシステムコール: 実行中タスクの終了
__noreturn void sys_task_exit(void) {
    arch_syscall(0, 0, 0, 0, 0, SYS_TASK_EXIT);
    UNREACHABLE();
}

// task_selfシステムコール: 実行中タスクのIDの取得
task_t sys_task_self(void) {
    return arch_syscall(0, 0, 0, 0, 0, SYS_TASK_SELF);
}

// pm_allocシステムコール: 物理メモリの割り当て
pfn_t sys_pm_alloc(task_t tid, size_t size, unsigned flags) {
    return arch_syscall(tid, size, flags, 0, 0, SYS_PM_ALLOC);
}

// vm_mapシステムコール: ページのマップ
error_t sys_vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, unsigned attrs) {
    return arch_syscall(task, uaddr, paddr, attrs, 0, SYS_VM_MAP);
}

// vm_unmapシステムコール: ページのアンマップ
error_t sys_vm_unmap(task_t task, uaddr_t uaddr) {
    return arch_syscall(task, uaddr, 0, 0, 0, SYS_VM_UNMAP);
}

// irq_listenシステムコール: 割り込み通知の購読
error_t sys_irq_listen(unsigned irq) {
    return arch_syscall(irq, 0, 0, 0, 0, SYS_IRQ_LISTEN);
}

// irq_unlistenシステムコール: 割り込み通知の購読解除
error_t sys_irq_unlisten(unsigned irq) {
    return arch_syscall(irq, 0, 0, 0, 0, SYS_IRQ_UNLISTEN);
}

// serial_writeシステムコール: 文字列の主力
int sys_serial_write(const char *buf, size_t len) {
    return arch_syscall((uintptr_t) buf, len, 0, 0, 0, SYS_SERIAL_WRITE);
}

// serial_readシステムコール: 文字入力の読み込み
int sys_serial_read(const char *buf, int max_len) {
    return arch_syscall((uintptr_t) buf, max_len, 0, 0, 0, SYS_SERIAL_READ);
}

// timeシステムコール: タイムアウトの設定
error_t sys_time(int milliseconds) {
    return arch_syscall(milliseconds, 0, 0, 0, 0, SYS_TIME);
}

// uptimeシステムコール: システムの起動時間の取得 (ミリ秒)
int sys_uptime(void) {
    return arch_syscall(0, 0, 0, 0, 0, SYS_UPTIME);
}

// shutdownシステムコール: システムのシャットダウン
__noreturn void sys_shutdown(void) {
    arch_syscall(0, 0, 0, 0, 0, SYS_SHUTDOWN);
    UNREACHABLE();
}
