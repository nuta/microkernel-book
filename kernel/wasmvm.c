#include "wasmvm.h"
#include "task.h"
#include "ipc.h"
#include <libs/common/string.h>
#include <libs/common/print.h>
#include <third_party/wasm-micro-runtime/core/iwasm/include/wasm_export.h>

// global pointer to curent task
struct task *current;

// global heap
static uint8_t global_heap_buf[512 * 1024] = {0};

// ipc wrappter
static error_t __ipc(task_t dst, task_t src, struct message *m, unsigned flags) {
    // validate src tid
    if (src < 0 || src > NUM_TASKS_MAX) {
        return ERR_INVALID_ARG;
    }

    // get destination if need
    struct task *dst_task = NULL;
    if (flags & IPC_SEND) {
        dst_task = task_find(dst);
        if (!dst_task) {
            WARN("%s: invalid task ID %d", current->name, dst);
            return ERR_INVALID_TASK;
        }
    }

    // make compiler happy
    return ipc(dst_task, src, (__user struct message *) m, flags | IPC_KERNEL | IPC_WASMVM);
}

// host functions exported to WASM
static void __ipc_reply(wasm_exec_env_t exec_env, task_t dst, struct message *m) {
    error_t err = __ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
    OOPS_OK(err);
}

static error_t __ipc_recv_any(struct message *m) {
    error_t err = __ipc(0, IPC_ANY, m, IPC_RECV);
    if (err != OK) {
        return err;
    }

    switch (m->type) {
        case NOTIFY_MSG: {
            int index = __builtin_ffsll(m->notify.notifications) - 1;
            switch (1 << index) {
                case NOTIFY_ASYNC_START ... NOTIFY_ASYNC_END: {
                    task_t src = index - NOTIFY_ASYNC_BASE;
                    m->type = ASYNC_RECV_MSG;
                    return __ipc(src, src, m, IPC_CALL);
                    break;
                }

                default:
                    PANIC(
                        "unhandled notification: %x (index=%d)", 
                        m->notify.notifications,
                        index
                    );
            }
        }

        default:
            if (IS_ERROR(m->type)) {
                return m->type;
            }
            return OK;
    }
}

static error_t __ipc_recv(wasm_exec_env_t exec_env, task_t src, struct message *m) {
    if (src == IPC_ANY) {
        return __ipc_recv_any(m);
    }
    return __ipc(0, src, m, IPC_RECV);
}

static error_t __ipc_call(wasm_exec_env_t exec_env, task_t dst, struct message *m) {
    return __ipc(dst, dst, m, IPC_CALL);
}

static task_t __ipc_lookup(wasm_exec_env_t exec_env, const char *name) {
    struct message m;
    m.type = SERVICE_LOOKUP_MSG;
    strcpy_safe(m.service_lookup.name, sizeof(m.service_lookup.name), name);
    
    ASSERT_OK(__ipc(VM_SERVER, VM_SERVER, &m, IPC_CALL));
    ASSERT(m.type == SERVICE_LOOKUP_REPLY_MSG);
    return m.service_lookup_reply.task;
}

__noreturn void wasmvm_run(struct wasmvm *wasmvm) {
    char error_buf[128];

    // set current task
    current = CURRENT_TASK;

    // init runtime
    static NativeSymbol native_symbols[] = {
        {"ipc_reply", __ipc_reply, "(i$)", NULL},
        {"ipc_recv", __ipc_recv, "(i$)i", NULL},
        {"ipc_call", __ipc_call, "(i$)i", NULL},
        {"ipc_lookup", __ipc_lookup, "($)i", NULL}
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

    // find main function
    wasm_function_inst_t func = NULL;
    func = wasm_runtime_lookup_function(moduleinst, "main", NULL);
    if(!func) {
        ERROR("main function not found\n");
        goto fail_3;
    }

    // create exec env
    wasm_exec_env_t exec_env = NULL;
    exec_env = wasm_runtime_create_exec_env(moduleinst, WASMVM_STACK_SIZE);
    if(!exec_env) {
        ERROR("create exec env failed\n");
        goto fail_3;
    }

    // invoke main function
    uint32_t argv[1] = {0};
    if (!wasm_runtime_call_wasm(exec_env, func, 0, argv)) {
        ERROR("call main failed");
    } else {
        TRACE("result: %d", argv[0]);
    }

    // destroy exec env
    wasm_runtime_destroy_exec_env(exec_env);

    fail_3:
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
 
