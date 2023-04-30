#pragma once
#include <libs/common/message.h>
#include <libs/common/types.h>

error_t ipc_send(task_t dst, struct message *m);
error_t ipc_send_noblock(task_t dst, struct message *m);
error_t ipc_send_async(task_t dst, struct message *m);
void ipc_reply(task_t dst, struct message *m);
void ipc_reply_err(task_t dst, error_t error);
error_t ipc_recv(task_t src, struct message *m);
error_t ipc_call(task_t dst, struct message *m);
error_t ipc_notify(task_t dst, notifications_t notifications);
error_t ipc_register(const char *name);
task_t ipc_lookup(const char *name);
