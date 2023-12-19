#include <libs/common/print.h>
#include "wasm_export.h"

unsigned char fib32_wasm[] = {
  0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
  0x01, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x07, 0x01, 0x03,
  0x66, 0x69, 0x62, 0x00, 0x00, 0x0a, 0x1f, 0x01, 0x1d, 0x00, 0x20, 0x00,
  0x41, 0x02, 0x49, 0x04, 0x40, 0x20, 0x00, 0x0f, 0x0b, 0x20, 0x00, 0x41,
  0x02, 0x6b, 0x10, 0x00, 0x20, 0x00, 0x41, 0x01, 0x6b, 0x10, 0x00, 0x6a,
  0x0f, 0x0b
};
unsigned int fib32_wasm_len = 62;

#define STACK_SIZE  (4096U)
#define HEAP_SIZE   (4096U)

int main(void) {
    // init runtime
    static char global_heap_buf[512 * 1024];
    char error_buf[128];

    RuntimeInitArgs init_args ={
        .mem_alloc_type = Alloc_With_Pool,
        .mem_alloc_option.pool.heap_buf = global_heap_buf,
        .mem_alloc_option.pool.heap_size = sizeof(global_heap_buf)
    };

    if(!wasm_runtime_full_init(&init_args)) {
        ERROR("Init runtime environment failed");
        goto fail;
    }

    // decode
    wasm_module_t module = NULL;
    module = wasm_runtime_load(
        fib32_wasm, 
        fib32_wasm_len, 
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

    wasm_exec_env_t exec_env = NULL;
    exec_env = wasm_runtime_create_exec_env(moduleinst, STACK_SIZE);
    if(!exec_env) {
        ERROR("Create wasm execution environment failed");
        goto fail;
    }

    // find function
    wasm_function_inst_t func = NULL;
    func = wasm_runtime_lookup_function(moduleinst, "fib", NULL);
    if(!func) {
        ERROR("fib function not found");
        goto fail;
    }

    // invoke
    wasm_val_t results[1] = {{.kind = WASM_I32, .of.i32 = 0}};
    wasm_val_t argments[1] = {
        {.kind = WASM_I32, .of.i32 = 24}
    };

    if(!wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, argments)) {
        ERROR("call wasm function fib failed");
        goto fail;
    }

    INFO("ret = %d", results[0].of.i32);

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