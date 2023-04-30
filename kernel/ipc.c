#include "ipc.h"
#include "syscall.h"
#include "task.h"
#include <libs/common/list.h>
#include <libs/common/message.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/common/types.h>

// メッセージの送信処理
static error_t send_message(struct task *dst, __user struct message *m,
                            unsigned flags) {
    // 自分自身にはメッセージを送信できない
    struct task *current = CURRENT_TASK;
    if (dst == current) {
        WARN("%s: tried to send a message to itself", current->name);
        return ERR_INVALID_ARG;
    }

    // 送信するメッセージをコピーする。ユーザーポインタの場合、ページフォルトが発生する可能性
    // があるので注意
    struct message copied_m;
    if (flags & IPC_KERNEL) {
        memcpy(&copied_m, (struct message *) m, sizeof(struct message));
    } else {
        error_t err = memcpy_from_user(&copied_m, m, sizeof(struct message));
        if (err != OK) {
            return err;
        }
    }

    // 送信先がメッセージを待っているか確認
    bool ready = dst->state == TASK_BLOCKED
                 && (dst->wait_for == IPC_ANY || dst->wait_for == current->tid);
    if (!ready) {
        if (flags & IPC_NOBLOCK) {
            return ERR_WOULD_BLOCK;
        }

        // 互いにメッセージを送り合おうとしている場合はデッドロックになるので、エラーを返す。
        LIST_FOR_EACH (task, &current->senders, struct task, waitqueue_next) {
            if (task->tid == dst->tid) {
                WARN(
                    "dead lock detected: %s (#%d) and %s (#%d) are trying to"
                    " send messages to each other"
                    " (hint: consider using ipc_send_async())",
                    current->name, current->tid, dst->name, dst->tid);
                return ERR_DEAD_LOCK;
            }
        }

        // 宛先の送信待ちキューに実行中タスクを追加し、ブロック状態にする
        list_push_back(&dst->senders, &current->waitqueue_next);
        task_block(current);

        // CPUを他のタスクに譲る。宛先タスクが受信状態になると、このタスクが再開される
        task_switch();

        // 宛先タスクが終了した場合は送信処理を中断する
        if (current->notifications & NOTIFY_ABORTED) {
            current->notifications &= ~NOTIFY_ABORTED;
            return ERR_ABORTED;
        }
    }

    // メッセージを送信して、宛先タスクを再開する
    memcpy(&dst->m, &copied_m, sizeof(struct message));
    dst->m.src = (flags & IPC_KERNEL) ? FROM_KERNEL : current->tid;
    task_resume(dst);
    return OK;
}

// メッセージの受信処理
static error_t recv_message(task_t src, __user struct message *m,
                            unsigned flags) {
    struct task *current = CURRENT_TASK;
    struct message copied_m;
    if (src == IPC_ANY && current->notifications) {
        // 通知がある場合は、それをメッセージとして受信する
        copied_m.type = NOTIFY_MSG;
        copied_m.src = FROM_KERNEL;
        copied_m.notify.notifications = current->notifications;
        current->notifications = 0;
    } else {
        if (flags & IPC_NOBLOCK) {
            return ERR_WOULD_BLOCK;
        }

        // 送信待ちキューに `src` に合致するタスクがあれば、それを再開する
        LIST_FOR_EACH (sender, &current->senders, struct task, waitqueue_next) {
            if (src == IPC_ANY || src == sender->tid) {
                DEBUG_ASSERT(sender->state == TASK_BLOCKED);
                DEBUG_ASSERT(sender->wait_for == IPC_DENY);
                list_remove(&sender->waitqueue_next);
                task_resume(sender);
                src = sender->tid;
                break;
            }
        }

        // メッセージを受信するまで待つ
        current->wait_for = src;
        task_block(current);
        task_switch();

        // メッセージを受け取った
        current->wait_for = IPC_DENY;
        memcpy(&copied_m, &current->m, sizeof(struct message));
    }

    // 受信したメッセージをコピーする。ユーザーポインタの場合、ページフォルトが発生する可能性
    // があるので注意
    if (flags & IPC_KERNEL) {
        memcpy((void *) m, &copied_m, sizeof(struct message));
    } else {
        error_t err = memcpy_to_user(m, &copied_m, sizeof(struct message));
        if (err != OK) {
            return err;
        }
    }

    return OK;
}

// メッセージを送受信する。
error_t ipc(struct task *dst, task_t src, __user struct message *m,
            unsigned flags) {
    // 送信操作
    if (flags & IPC_SEND) {
        error_t err = send_message(dst, m, flags);
        if (err != OK) {
            return err;
        }
    }

    // 受信操作
    if (flags & IPC_RECV) {
        error_t err = recv_message(src, m, flags);
        if (err != OK) {
            return err;
        }
    }

    return OK;
}

// 通知を送信する。
void notify(struct task *dst, notifications_t notifications) {
    if (dst->state == TASK_BLOCKED && dst->wait_for == IPC_ANY) {
        // 宛先タスクがオープン受信状態で待っている。NOTIFY_MSGメッセージを送った体で
        // 通知を即座に配送する。
        dst->m.type = NOTIFY_MSG;
        dst->m.src = FROM_KERNEL;
        dst->m.notify.notifications = dst->notifications | notifications;
        dst->notifications = 0;
        task_resume(dst);
    } else {
        // 宛先タスクがオープン受信をするまで通知を保留する。
        dst->notifications |= notifications;
    }
}
