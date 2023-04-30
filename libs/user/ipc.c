#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>
#include <libs/user/syscall.h>
#include <libs/user/task.h>

// 非同期メッセージ
struct async_message {
    list_elem_t next;  // 送信キューのリスト
    task_t dst;        // 宛先タスク
    struct message m;  // メッセージ
};

// このタスクから他のタスクに向けて送信される非同期メッセージリスト。
// 他のタスクから問い合わせ (ASYNC_RECV_MSG) があると、このリストからメッセージを探す。
static list_t async_messages = LIST_INIT(async_messages);
// 受信済みの通知 (ビットフィールド)。
static notifications_t pending_notifications = 0;

// ASYNC_RECV_MSGを受信した際の処理 (ノンブロッキング)
static error_t async_reply(task_t dst) {
    // 送信キューからdstへの未送信メッセージを探す
    bool sent = false;
    LIST_FOR_EACH (am, &async_messages, struct async_message, next) {
        if (am->dst == dst) {
            if (sent) {
                // 既にメッセージを1つ送信済みであればipc_replyが失敗してしまう
                // (宛先タスクが受信待ち状態でない) ため、通知を送っておいて
                // 再度ASYNC_RECV_MSGを送らせる。
                return ipc_notify(dst, NOTIFY_ASYNC(task_self()));
            }

            // 未送信メッセージを返信する
            ipc_reply(dst, &am->m);
            list_remove(&am->next);
            free(am);
            sent = true;
        }
    }

    return OK;
}

// 非同期メッセージを送信する (ノンブロッキング)
error_t ipc_send_async(task_t dst, struct message *m) {
    // メッセージを送信キューに挿入する
    struct async_message *am = malloc(sizeof(*am));
    am->dst = dst;
    memcpy(&am->m, m, sizeof(am->m));
    list_elem_init(&am->next);
    list_push_back(&async_messages, &am->next);

    // 送信先タスクに通知を送る
    return ipc_notify(dst, NOTIFY_ASYNC(task_self()));
}

// メッセージを送信する。宛先タスクが受信状態になるまでブロックする。
error_t ipc_send(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND);
}

// メッセージを送信する。即座にメッセージ送信を完了できない場合は ERR_WOULD_BLOCK を返す。
error_t ipc_send_noblock(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
}

// メッセージを送信する。即座にメッセージ送信を完了できない場合は警告メッセージを出力し、
// メッセージを破棄する。
void ipc_reply(task_t dst, struct message *m) {
    error_t err = ipc_send_noblock(dst, m);
    OOPS_OK(err);
}

// エラーメッセージを送信する。即座にメッセージ送信を完了できない場合は警告メッセージを出力し、
// メッセージを破棄する。
void ipc_reply_err(task_t dst, error_t error) {
    struct message m;
    m.type = error;
    ipc_reply(dst, &m);
}

// 受信済み通知のひとつを取り出し、メッセージに変換する。また、非同期メッセージの受信処理も
// 透過的に行う。
static error_t recv_notification_as_message(struct message *m) {
    error_t err;

    // 受信済み通知ビットフィールドの中で何番目のビットが立っているかを調べる。
    int index = __builtin_ffsll(pending_notifications) - 1;
    DEBUG_ASSERT(index >= 0);

    // 通知の種類に応じてメッセージを作成する。
    switch (1 << index) {
        // 割り込み通知
        case NOTIFY_IRQ:
            m->type = NOTIFY_IRQ_MSG;
            err = OK;
            break;
        // タイムアウト通知
        case NOTIFY_TIMER:
            m->type = NOTIFY_TIMER_MSG;
            err = OK;
            break;
        // 非同期メッセージ受信通知
        case NOTIFY_ASYNC_START ... NOTIFY_ASYNC_END: {
            // 通知の送信元に対して受信待ちメッセージを問い合わせる
            task_t src = index - NOTIFY_ASYNC_BASE;
            m->type = ASYNC_RECV_MSG;
            err = ipc_call(src, m);
            break;
        }
        case NOTIFY_ABORTED:
            // カーネル内部で使われる通知なので、ここには来ないはず。
        default:
            PANIC("unhandled notification: %x (index=%d)",
                  pending_notifications, index);
    }

    // 処理した通知を削除する
    pending_notifications &= ~(1 << index);
    return err;
}

// 任意のタスクからのメッセージを受信する (オープン受信)。通知・非同期メッセージパッシング周り
// の処理も透過的に行う。
static error_t ipc_recv_any(struct message *m) {
    while (true) {
        // 受信済み通知があれば、その通知をメッセージに変換して返す。
        if (pending_notifications) {
            return recv_notification_as_message(m);
        }

        // メッセージを受信する。
        error_t err = sys_ipc(0, IPC_ANY, m, IPC_RECV);
        if (err != OK) {
            return err;
        }

        // メッセージの種類に応じた処理を行う。
        switch (m->type) {
            // 通知処理: 通知を受信済み通知ビットフィールドに追加する。
            case NOTIFY_MSG:
                if (m->src != FROM_KERNEL) {
                    WARN(
                        "received a notification from a non-kernel task #%d, ignoring",
                        m->src);
                    continue;
                }

                pending_notifications |= m->notify.notifications;
                return recv_notification_as_message(m);
            // 非同期メッセージ問い合わせ処理: 送信元タスクへの非同期メッセージがあれば返す。
            case ASYNC_RECV_MSG: {
                error_t err = async_reply(m->src);
                if (err != OK) {
                    WARN("failed to send a async message to #%d: %s", m->src,
                         err2str(err));
                }
                continue;
            }
            // その他のメッセージ: エラーでなければそのまま返す。
            default:
                if (IS_ERROR(m->type)) {
                    return m->type;
                }

                return OK;
        }
    }
}

// メッセージを受信する。メッセージが届くまでブロックする。
//
// src が IPC_ANY の場合は、任意のタスクからのメッセージを受信する (オープン受信)。
error_t ipc_recv(task_t src, struct message *m) {
    if (src == IPC_ANY) {
        // オープン受信
        return ipc_recv_any(m);
    }

    // クローズド受信
    error_t err = sys_ipc(0, src, m, IPC_RECV);
    if (err != OK) {
        return err;
    }

    // エラーメッセージが返ってくれば、そのエラーを返す。
    if (IS_ERROR(m->type)) {
        return m->type;
    }

    return OK;
}

// メッセージを送信し、その宛先からのメッセージを待つ。
error_t ipc_call(task_t dst, struct message *m) {
    error_t err = sys_ipc(dst, dst, m, IPC_CALL);
    if (err != OK) {
        return err;
    }

    // エラーメッセージが返ってくれば、そのエラーを返す。
    if (IS_ERROR(m->type)) {
        return m->type;
    }

    return OK;
}

// 通知を送信する。
error_t ipc_notify(task_t dst, notifications_t notifications) {
    return sys_notify(dst, notifications);
}

// サービスを登録する。
error_t ipc_register(const char *name) {
    struct message m;
    m.type = SERVICE_REGISTER_MSG;
    strcpy_safe(m.service_register.name, sizeof(m.service_register.name), name);
    return ipc_call(VM_SERVER, &m);
}

// サービス名からタスクIDを検索する。サービスが登録されるまでブロックする。
task_t ipc_lookup(const char *name) {
    struct message m;
    m.type = SERVICE_LOOKUP_MSG;
    strcpy_safe(m.service_lookup.name, sizeof(m.service_lookup.name), name);
    error_t err = ipc_call(VM_SERVER, &m);
    if (err != OK) {
        return err;
    }

    ASSERT(m.type == SERVICE_LOOKUP_REPLY_MSG);
    return m.service_lookup_reply.task;
}
