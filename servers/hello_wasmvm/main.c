#include <libs/common/print.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>
#include <libs/user/task.h>

// defined in wasm.S
extern uint8_t wasm_start[];
extern uint32_t wasm_size[];

void main(void) {
    task_t server = sys_wasmvm("wasm_hello", wasm_start, wasm_size[0], task_self());
    ASSERT_OK(server);

    TRACE("server = %x", server);
    
    // send message to WASMVM task
    struct message m;
    m.type = PING_MSG;
    m.ping.value = 123;
    ASSERT_OK(ipc_call(server, &m));

    // display message from WASMVM task
    INFO("reply type: %s", msgtype2str(m.type));
    INFO("reply value: %d", m.ping_reply.value);

    // destroy WASMVM task
    ASSERT_OK(sys_task_destroy(server));
}