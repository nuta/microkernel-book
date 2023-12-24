#include "wasmvm.h"
#include "task.h"
#include "ipc.h"
#include <libs/common/print.h>
#include <libs/third_party/wasm-micro-runtime/core/iwasm/include/wasm_export.h>

// global pointer to curent task
struct task *current;

// global heap
static uint8_t global_heap_buf[512 * 1024] = {0};

// host functions exported to WASM
static void __info(wasm_exec_env_t exec_env, const char *str) {
    INFO("%s", str);
}

static void __ipc_reply(wasm_exec_env_t exec_env, task_t dst_tid, struct message *m) {
    // find destination task
    struct task *dst = task_find(dst_tid);
    if (dst == NULL) {
        WARN("%s: REPLY: invalid task ID %d", current->name, dst);
        task_exit(EXP_ILLEGAL_EXCEPTION);
    }

    error_t err = ipc(dst, 0, (__user struct message *) m, 
                      IPC_SEND | IPC_NOBLOCK | IPC_KERNEL);
    OOPS_OK(err);
}

static error_t __ipc_recv(wasm_exec_env_t exec_env, task_t src_tid, struct message *m) {
    return ipc(0, src_tid, (__user struct message *) m, IPC_RECV | IPC_KERNEL);
}

__noreturn void wasmvm_run(struct wasmvm *wasmvm) {
    char error_buf[128];

    // set current task
    current = CURRENT_TASK;

    // init runtime
    static NativeSymbol native_symbols[] = {
        {"info", __info, "($)", NULL},
        {"ipc_recv", __ipc_recv, "(i$)i", NULL},
        {"ipc_reply", __ipc_reply, "(i$)", NULL}
    };

    RuntimeInitArgs init_args ={
        .mem_alloc_type = Alloc_With_Pool,
        .mem_alloc_option.pool.heap_buf = global_heap_buf,
        .mem_alloc_option.pool.heap_size = sizeof(global_heap_buf),
        .n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol),
        .native_module_name = "env",
        .native_symbols = native_symbols
    };

    if (!wasm_runtime_full_init(&init_args)) {
        ERROR("init runtime environment failed");
        task_exit(EXP_GRACE_EXIT);
    }

    wasm_module_t module = NULL;
    module = wasm_runtime_load(
        wasmvm->code, 
        wasmvm->size, 
        error_buf, 
        sizeof(error_buf)
    );
    if (!module) {
        ERROR("load wasm module failed");
        goto fail_1;
    }

    // instantiate
    wasm_module_inst_t moduleinst = NULL;
    moduleinst = wasm_runtime_instantiate(
        module,
        WASMVM_STACK_SIZE,
        WASMVM_HEAP_SIZE,
        error_buf,
        sizeof(error_buf)
    );
    if(!moduleinst) {
        ERROR("instantiation failed");
        goto fail_2;
    }

    char *argv[] = {NULL};
    if (!wasm_application_execute_main(moduleinst, 0, argv)) {
        ERROR("main function failed");
    }

    INFO("ret = %d", *(int *)argv);

    // destroy the module instance  
    wasm_runtime_deinstantiate(moduleinst);
    
    fail_2:
        // unload the module
        wasm_runtime_unload(module);
    fail_1:
        // destroy runtime environment
        wasm_runtime_destroy();
    
    // exit
    task_exit(EXP_GRACE_EXIT);
}
 
