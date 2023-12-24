#include <libs/common/print.h>
#include <libs/user/syscall.h>
#include <libs/user/task.h>

// defined in wasm.S
extern uint8_t wasm_start[];
extern uint32_t wasm_size[];

void main(void) {
    task_t server = sys_wasmvm("wasm_hello", wasm_start, wasm_size[0], task_self());
    ASSERT_OK(server);

    NYI();
}