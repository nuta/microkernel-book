#pragma once
#include "ipcstub.h"

#define IPC_ANY  0
#define IPC_DENY -1

#define IPC_SEND    (1 << 16)
#define IPC_RECV    (1 << 17)
#define IPC_NOBLOCK (1 << 18)
#define IPC_KERNEL  (1 << 19)
#define IPC_WASMVM  (1 << 20)
#define IPC_CALL    (IPC_SEND | IPC_RECV)

#define NOTIFY_TIMER       (1 << 0)
#define NOTIFY_IRQ         (1 << 1)
#define NOTIFY_ABORTED     (1 << 2)
#define NOTIFY_ASYNC_BASE  3
#define NOTIFY_ASYNC(tid)  (1 << (NOTIFY_ASYNC_BASE + tid))
#define NOTIFY_ASYNC_START (NOTIFY_ASYNC(0))
#define NOTIFY_ASYNC_END   (NOTIFY_ASYNC(NUM_TASKS_MAX))

// 各タスクは専用のASYNC用ビットフィールドをnotification_tに持つ
STATIC_ASSERT(NOTIFY_ASYNC_BASE + NUM_TASKS_MAX < sizeof(notifications_t) * 8,
              "too many tasks for notifications_t");

struct message {
    int32_t type;  // メッセージの種類 (負の数の場合はエラー値)
    task_t src;    // メッセージの送信元
    union {
        uint8_t data[0];  // メッセージデータの先頭を指す
        /// 自動生成される各メッセージのフィールド定義:
        //
        //     struct { int x; int y; } add;
        //     struct { int answer; } add_reply;
        //     ...
        //
        IPCSTUB_MESSAGE_FIELDS
    };
};

STATIC_ASSERT(sizeof(struct message) < 2048,
              "sizeof(struct message) too large");

const char *msgtype2str(int type);
