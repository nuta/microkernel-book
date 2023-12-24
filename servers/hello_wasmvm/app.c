#include <kernel/wasmvm.h>

#define ASSERT_OK(expr)                                                         \
    do {                                                                        \
        __typeof__(expr) __expr = (expr);                                       \
        if (__expr < 0) {                                                       \
            return -1;                                                          \
        }                                                                       \
    } while(0)

__attribute__((export_name("main")))
int main(void) {
    while(1) {
        struct message m;
        ASSERT_OK(ipc_recv(IPC_ANY, &m));
        switch(m.type) {
            case PING_MSG: {
                m.type = PING_REPLY_MSG;
                m.ping_reply.value = 42;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                break;
        }
    }
}