#pragma once

#include <libs/common/types.h>
#include <libs/common/message.h>

// host functions exported to WASM (defined in kernel/wasmvm.c)
void ipc_reply(task_t dst, struct message *m);
error_t ipc_recv(task_t src, struct message *m);
error_t ipc_call(task_t dst, struct message *m);
task_t ipc_lookup(const char *name);