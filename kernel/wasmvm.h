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

// entry point of WASMVM task
__noreturn void wasmvm_run(struct wasmvm *wasmvm);