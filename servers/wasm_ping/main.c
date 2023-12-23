#include <libs/common/print.h>
#include <libs/user/ipc.h>
#include "wasm_export.h"

#define STACK_SIZE  (4096U)
#define HEAP_SIZE   (4096U)

// defined in main.S
extern unsigned char wasm_start[];
extern int wasm_len[];

// host function
task_t host_ipc_lookup(wasm_exec_env_t exec_env, const char *name) {
    return ipc_lookup(name);
}

error_t host_ipc_call(wasm_exec_env_t exec_env, task_t dst, struct message *m) {
    return ipc_call(dst, m);
}

int main(void) {
    // init runtime
    static char global_heap_buf[512 * 1024];
    char error_buf[128];

    // register host functon
    static NativeSymbol native_symbols[] = {
        {"ipc_lookup", host_ipc_lookup, "($)i"},
        {"ipc_call", host_ipc_call, "(i$)i"}
    };

    RuntimeInitArgs init_args ={
        .mem_alloc_type = Alloc_With_Pool,
        .mem_alloc_option.pool.heap_buf = global_heap_buf,
        .mem_alloc_option.pool.heap_size = sizeof(global_heap_buf),
        .n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol),
        .native_module_name = "env",
        .native_symbols = native_symbols
    };

    if(!wasm_runtime_full_init(&init_args)) {
        ERROR("Init runtime environment failed");
        goto fail;
    }

    // decode
    wasm_module_t module = NULL;
    module = wasm_runtime_load(
        wasm_start, 
        wasm_len[0], 
        error_buf, 
        sizeof(error_buf)
    );
    if (!module) {
        ERROR("Load wasm module failed");
        goto fail;
    }

    // instantiate
    wasm_module_inst_t moduleinst = NULL;
    moduleinst = wasm_runtime_instantiate(
        module,
        STACK_SIZE,
        HEAP_SIZE,
        error_buf,
        sizeof(error_buf)
    );
    if(!moduleinst) {
        ERROR("Instantiation failed");
        goto fail;
    }

    // find function
    wasm_function_inst_t func = NULL;
    func = wasm_runtime_lookup_function(moduleinst, "main", NULL);
    if(!func) {
        ERROR("main function not found");
        goto fail;
    }

    // create exec environment
    wasm_exec_env_t exec_env = NULL;
    exec_env = wasm_runtime_create_exec_env(moduleinst, STACK_SIZE);
    if(!exec_env) {
        ERROR("Create wasm execution environment failed");
        goto fail;
    }

    // invoke
    uint32_t args[1] = {0};
    if(!wasm_runtime_call_wasm(exec_env, func, 0, args)) {
        ERROR("call wasm function main failed");
        goto fail;
    }

    INFO("ret = %d", args[0]);

    // cleanup
    fail:
        if (exec_env)
            wasm_runtime_destroy_exec_env(exec_env);
        if (moduleinst)
            wasm_runtime_deinstantiate(moduleinst);
        if (module)
            wasm_runtime_unload(module);
        wasm_runtime_destroy();
        
        return 0;
}