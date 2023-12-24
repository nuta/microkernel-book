#include "wasmvm.h"
#include "task.h"
#include <libs/third_party/wasm-micro-runtime/core/iwasm/include/wasm_export.h>
#include <libs/common/print.h>

// global heap
static uint8_t global_heap_buf[512 * 1024] = {0};

// host functions exported to WASM
static void info(wasm_exec_env_t exec_env, const char *str) {
    INFO("%s", str);
}

__noreturn void wasmvm_run(struct wasmvm *wasmvm) {
    char error_buf[128];

    // init runtime
    static NativeSymbol native_symbols[] = {
        {"info", info, "($)", NULL},
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
 
