#include "interrupt.h"
#include "arch.h"
#include "ipc.h"
#include "task.h"
#include <libs/common/print.h>

// 割り込み通知を受け付けるタスクの一覧。
static struct task *irq_listeners[IRQ_MAX];
// 起動してからの経過時間。単位はタイマー割り込みの周期 (TICK_HZ) に依存する。
unsigned uptime_ticks = 0;

// 割り込み通知を受け付けるようにする。
error_t irq_listen(struct task *task, unsigned irq) {
    if (irq >= IRQ_MAX) {
        return ERR_INVALID_ARG;
    }

    if (irq_listeners[irq] != NULL) {
        return ERR_ALREADY_USED;
    }

    error_t err = arch_irq_enable(irq);
    if (err != OK) {
        return err;
    }

    irq_listeners[irq] = task;
    return OK;
}

// 割り込み通知を受け付けないようにする。
error_t irq_unlisten(struct task *task, unsigned irq) {
    if (irq >= IRQ_MAX) {
        return ERR_INVALID_ARG;
    }

    if (irq_listeners[irq] != task) {
        return ERR_NOT_ALLOWED;
    }

    error_t err = arch_irq_disable(irq);
    if (err != OK) {
        return err;
    }

    irq_listeners[irq] = NULL;
    return OK;
}

// ハードウェア割り込みハンドラ (タイマー割り込み以外)
void handle_interrupt(unsigned irq) {
    if (irq >= IRQ_MAX) {
        WARN("invalid IRQ: %u", irq);
        return;
    }

    // 割り込みを受け付けるタスクを取得し、通知を送る。
    struct task *task = irq_listeners[irq];
    if (!task) {
        WARN("unhandled IRQ %u", irq);
        return;
    }

    notify(task, NOTIFY_IRQ);
}

// タイマー割り込みハンドラ
void handle_timer_interrupt(unsigned ticks) {
    // 起動してからの経過時間を更新
    uptime_ticks += ticks;

    if (CPUVAR->id == 0) {
        // 各タスクのタイマーを更新する
        LIST_FOR_EACH (task, &active_tasks, struct task, next) {
            if (task->timeout > 0) {
                task->timeout -= MIN(task->timeout, ticks);
                if (!task->timeout) {
                    // タイムアウトしたのでタスクに通知する
                    notify(task, NOTIFY_TIMER);
                }
            }
        }
    }

    // 実行中タスクの残り実行可能時間を更新し、ゼロになったらタスク切り替えを行う
    struct task *current = CURRENT_TASK;
    DEBUG_ASSERT(current->quantum >= 0 || current == IDLE_TASK);
    current->quantum -= MIN(ticks, current->quantum);
    if (!current->quantum) {
        task_switch();
    }
}
