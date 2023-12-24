#include "task.h"
#include "arch.h"
#include "ipc.h"
#include "memory.h"
#include "printk.h"
#include <libs/common/list.h>
#include <libs/common/string.h>

static struct task tasks[NUM_TASKS_MAX];        // 全てのタスク管理構造体 (未使用含む)
static struct task idle_tasks[NUM_CPUS_MAX];    // 各CPUのアイドルタスク
static list_t runqueue = LIST_INIT(runqueue);   // ランキュー
list_t active_tasks = LIST_INIT(active_tasks);  // 使用中の管理構造体のリスト

// 次に実行するタスクを選択する。
static struct task *scheduler(void) {
    // ランキューから実行可能なタスクを取り出す。
    struct task *next = LIST_POP_FRONT(&runqueue, struct task, waitqueue_next);
    if (next) {
        return next;
    }

    if (CURRENT_TASK->state == TASK_RUNNABLE && !CURRENT_TASK->destroyed) {
        // 他に実行可能なタスクがない場合は、実行中タスクを続行する。
        return CURRENT_TASK;
    }

    return IDLE_TASK;  // 実行するタスクがない場合はアイドルタスクを実行する。
}

// タスク管理構造体を初期化する。
static error_t init_task_struct(struct task *task, task_t tid, const char *name,
                                vaddr_t ip, struct task *pager,
                                vaddr_t kernel_entry, void *arg) {
    task->tid = tid;
    task->destroyed = false;
    task->quantum = 0;
    task->timeout = 0;
    task->wait_for = IPC_DENY;
    task->ref_count = 0;
    task->pager = pager;

    strcpy_safe(task->name, sizeof(task->name), name);
    list_elem_init(&task->waitqueue_next);
    list_elem_init(&task->next);
    list_init(&task->senders);
    list_init(&task->pages);

    error_t err = arch_vm_init(&task->vm);
    if (err != OK) {
        return err;
    }

    err = arch_task_init(task, ip, kernel_entry, arg);
    if (err != OK) {
        arch_vm_destroy(&task->vm);
        return err;
    }

    if (pager) {
        pager->ref_count++;
    }

    task->state = TASK_BLOCKED;
    return OK;
}

// 自発的なタスク切り替えを行う。もし実行可能なタスクが実行中タスク以外にない場合は、即座に
// 戻ってくる。その他の場合は、他のタスクに実行が移され、次回タスクが再びスケジュールされたとき
// に戻ってくる。
void task_switch(void) {
    struct task *prev = CURRENT_TASK;  // 実行中タスク
    struct task *next = scheduler();   // 次に実行するタスク

    // 次に実行するタスクにCPU時間を与える
    if (next != IDLE_TASK) {
        next->quantum = TASK_QUANTUM;
    }

    if (next == prev) {
        // 実行中タスク以外に実行可能なタスクがない。戻って処理を続ける。
        return;
    }

    if (prev->state == TASK_RUNNABLE) {
        // 実行中タスクが実行可能な状態ならば、実行可能なタスクのキューに戻す。
        // 与えられたCPU時間を使い切ったときに起きる。
        list_push_back(&runqueue, &prev->waitqueue_next);
    }

    // タスクを切り替える
    CURRENT_TASK = next;
    arch_task_switch(prev, next);
}

// 未使用のタスクIDを探す。
static task_t alloc_tid(void) {
    for (task_t i = 0; i < NUM_TASKS_MAX; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            return i + 1;
        }
    }

    return 0;
}

// タスクIDからタスク管理構造体を取得する。存在しない場合や無効なIDの場合はNULLを返す。
struct task *task_find(task_t tid) {
    if (tid < 0 || tid >= NUM_TASKS_MAX) {
        return NULL;
    }

    struct task *task = &tasks[tid - 1];
    if (task->state == TASK_UNUSED) {
        return NULL;
    }

    return task;
}

// タスクをブロック状態にする。実行中タスク自身をブロックする場合は、task_switch関数を
// 呼び出して他のタスクに実行を移す必要がある。
void task_block(struct task *task) {
    DEBUG_ASSERT(task != IDLE_TASK);
    DEBUG_ASSERT(task->state == TASK_RUNNABLE);

    task->state = TASK_BLOCKED;
}

// タスクを実行可能状態にする。
void task_resume(struct task *task) {
    DEBUG_ASSERT(task->state == TASK_BLOCKED);

    task->state = TASK_RUNNABLE;
    list_push_back(&runqueue, &task->waitqueue_next);
}

// タスクを作成する。ipはユーザーモードで実行するアドレス (エントリーポイント)、pagerは
// ページャータスク。
task_t task_create(const char *name, uaddr_t ip, struct task *pager) {
    task_t tid = alloc_tid();
    if (!tid) {
        return ERR_TOO_MANY_TASKS;
    }

    struct task *task = &tasks[tid - 1];
    DEBUG_ASSERT(task != NULL);

    error_t err = init_task_struct(task, tid, name, ip, pager, 0, NULL);
    if (err != OK) {
        return err;
    }

    list_push_back(&active_tasks, &task->next);
    task_resume(task);
    TRACE("created a task \"%s\" (tid=%d)", name, tid);
    return tid;
}

// HinaVMタスクを作成する。instsはHinaVM命令列、num_instsは命令数、pagerはページャータスク。
// hinavm.c ではなくここで書かれているのは、init_task_struct関数などを呼び出すため。
task_t hinavm_create(const char *name, hinavm_inst_t *insts, uint32_t num_insts,
                     struct task *pager) {
    task_t tid = alloc_tid();
    if (!tid) {
        return ERR_TOO_MANY_TASKS;
    }

    struct task *task = &tasks[tid - 1];
    DEBUG_ASSERT(task != NULL);

    size_t hinavm_size = ALIGN_UP(sizeof(struct hinavm), PAGE_SIZE);
    paddr_t hinavm_paddr = pm_alloc(hinavm_size, NULL, PM_ALLOC_UNINITIALIZED);
    if (!hinavm_paddr) {
        return ERR_NO_MEMORY;
    }

    struct hinavm *hinavm = (struct hinavm *) arch_paddr_to_vaddr(hinavm_paddr);
    memcpy(&hinavm->insts, insts, sizeof(hinavm_inst_t) * num_insts);
    hinavm->num_insts = num_insts;

    error_t err = init_task_struct(task, tid, name, 0, pager,
                                   (vaddr_t) hinavm_run, hinavm);
    if (err != OK) {
        pm_free(hinavm_paddr, hinavm_size);
        return err;
    }

    pm_own_page(hinavm_paddr, task);
    list_push_back(&active_tasks, &task->next);
    task_resume(task);
    TRACE("created a HinaVM task \"%s\" (tid=%d)", name, tid);
    return tid;
}

// Create WASMVM task
task_t wasmvm_create(const char *name, uint8_t *wasm, uint32_t size, 
                     struct task *pager) {
    task_t tid = alloc_tid();
    if (!tid) {
        return ERR_TOO_MANY_TASKS;
    }

    struct task *task = &tasks[tid - 1];
    DEBUG_ASSERT(task != NULL);

    // allocate physical memory
    size_t wasmvm_size = ALIGN_UP(sizeof(struct wasmvm), PAGE_SIZE);
    paddr_t wasmvm_paddr = pm_alloc(wasmvm_size, NULL, PM_ALLOC_UNINITIALIZED);
    if (!wasmvm_paddr) {
        return ERR_NO_MEMORY;
    }

    // copy wasm binary
    struct wasmvm *wasmvm = (struct wasmvm *) arch_paddr_to_vaddr(wasmvm_paddr);
    memcpy(&wasmvm->code, wasm, size);
    wasmvm->size = size;
    
    error_t err = init_task_struct(task, tid, name, 0, pager, 
                                    (vaddr_t) wasmvm_run, wasmvm);
    
    if (err != OK) {
        pm_free(wasmvm_paddr, wasmvm_size);
        return err;
    }

    pm_own_page(wasmvm_paddr, task);
    list_push_back(&active_tasks, &task->next);
    task_resume(task);
    TRACE("created a WASMVM task \"%s\" (tid=%d)", name, tid);
    return tid;
}

// タスクを削除する。taskは削除するタスク。taskが実行中のタスクである場合は、この関数では
// なく、task_exit関数を呼び出す必要がある。
error_t task_destroy(struct task *task) {
    DEBUG_ASSERT(task != CURRENT_TASK);
    DEBUG_ASSERT(task != IDLE_TASK);
    DEBUG_ASSERT(task->state != TASK_UNUSED);
    DEBUG_ASSERT(task->ref_count >= 0);

    if (task->tid == 1) {
        // 最初のユーザータスク (VMサーバ) は削除できない。
        WARN("tried to destroy the task #1");
        return ERR_INVALID_ARG;
    }

    if (task->ref_count > 0) {
        // 他のタスクから参照されている場合 (他のタスクのページャータスクとして登録されている)
        // は削除できない。
        WARN("%s (#%d) is still referenced from %d tasks", task->name,
             task->tid, task->ref_count);
        return ERR_STILL_USED;
    }

    TRACE("destroying a task \"%s\" (tid=%d)", task->name, task->tid);

    // 削除中であること記録しておくことで、以下のプロセッサ間割り込みを受け取った他のCPUでの
    // スケジューラが再びこのタスクを選ばないようにする。こうしておかないと、もしこのタスク以外
    // に実行可能なタスクが存在しない場合に以下のループが永久に実行される恐れがある。
    task->destroyed = true;

    // 他のCPUがこのタスクの実行を中断するまで待つ。
    while (true) {
        // タスクがブロックされていれば明らかに現在実行中ではない。
        if (task->state != TASK_RUNNABLE) {
            break;
        }

        // タスクが実行可能状態であってもランキューに含まれていなければ現在実行中ではない。
        if (list_contains(&runqueue, &task->waitqueue_next)) {
            break;
        }

        // 他のCPUがこのタスクを実行中である。IPIを送信してコンテキストスイッチを促す。
        arch_send_ipi(IPI_RESCHEDULE);
    }

    // もしこのタスクへメッセージを送ろうとしているタスクがいたら、それらの送信処理を中断させる。
    LIST_FOR_EACH (sender, &task->senders, struct task, waitqueue_next) {
        notify(sender, NOTIFY_ABORTED);
    }

    // カーネルからタスクを削除する。
    list_remove(&task->next);
    list_remove(&task->waitqueue_next);
    arch_vm_destroy(&task->vm);
    arch_task_destroy(task);
    pm_free_by_list(&task->pages);
    task->state = TASK_UNUSED;
    task->pager->ref_count--;
    return OK;
}

// 実行中タスク (CURRENT_TASK) を終了させる。引数exceptionは終了理由。
__noreturn void task_exit(int exception) {
    struct task *pager = CURRENT_TASK->pager;
    ASSERT(pager != NULL);

    TRACE("exiting a task \"%s\" (tid=%d)", CURRENT_TASK->name,
          CURRENT_TASK->tid);

    // ページャータスクに終了理由を通知する。ページャータスクがtask_destroyシステムコールを
    // 呼び出すことで、このタスクが実際に削除される。
    struct message m;
    m.type = EXCEPTION_MSG;
    m.exception.task = CURRENT_TASK->tid;
    m.exception.reason = exception;
    error_t err = ipc(CURRENT_TASK->pager, IPC_DENY,
                      (__user struct message *) &m, IPC_SEND | IPC_KERNEL);

    if (err != OK) {
        WARN("%s: failed to send an exit message to '%s': %s",
             CURRENT_TASK->name, pager->name, err2str(err));
    }

    // 他のタスクを実行する。もうこのタスクに戻ってくることはない。
    task_block(CURRENT_TASK);
    task_switch();

    UNREACHABLE();
}

// デバッグ用に現在の各タスクの状態を表示する。デッドロックが起きた場合の原因究明に便利。
// シリアルポートで Ctrl-P が押されると呼び出される。
void task_dump(void) {
    WARN("active tasks:");
    LIST_FOR_EACH (task, &active_tasks, struct task, next) {
        switch (task->state) {
            case TASK_RUNNABLE:
                WARN("  #%d: %s: RUNNABLE", task->tid, task->name);
                LIST_FOR_EACH (sender, &task->senders, struct task,
                               waitqueue_next) {
                    WARN("    blocked sender: #%d: %s", sender->tid,
                         sender->name);
                }
                break;
            case TASK_BLOCKED:
                switch (task->wait_for) {
                    case IPC_DENY:
                        WARN(
                            "  #%d: %s: BLOCKED (send, serial_read, or exited)",
                            task->tid, task->name);
                        break;
                    case IPC_ANY:
                        WARN("  #%d: %s: BLOCKED (open receive)", task->tid,
                             task->name);
                        break;
                    default:
                        WARN("  #%d: %s: BLOCKED (closed receive from #%d)",
                             task->tid, task->name, task->wait_for);
                }
                break;
            default:
                UNREACHABLE();
        }
    }
}

// タスク管理システムの初期化
void task_init_percpu(void) {
    // CPUごとのアイドルタスクを作成し、それを実行中タスクとする。
    struct task *idle_task = &idle_tasks[CPUVAR->id];
    ASSERT_OK(init_task_struct(idle_task, 0, "(idle)", 0, NULL, 0, NULL));
    IDLE_TASK = idle_task;
    CURRENT_TASK = IDLE_TASK;
}
