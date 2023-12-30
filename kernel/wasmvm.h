#pragma once

#include <libs/common/types.h>
#include <libs/common/message.h>

#define WASMVM_CODE_SIZE_MAX    4096
#define WASMVM_STACK_SIZE       4096
#define WASMVM_HEAP_SIZE        4096

struct wasmvm {
    uint8_t     code[WASMVM_CODE_SIZE_MAX];
    uint32_t    size;
};

// host functions exported to WASM (defined in wasmvm.c)
void ipc_reply(task_t dst, struct message *m);
error_t ipc_recv(task_t src, struct message *m);
error_t ipc_call(task_t dst, struct message *m);
task_t ipc_lookup(const char *name);

// entry point of WASMVM task
__noreturn void wasmvm_run(struct wasmvm *wasmvm);