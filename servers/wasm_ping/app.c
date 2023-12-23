#include <libs/user/ipc.h>

__attribute__((export_name("main")))
int main(void) {
    task_t server = ipc_lookup("pong");
    struct message m;

    m.type = PING_MSG;
    m.ping.value = 123;

    if (IS_ERROR(ipc_call(server, &m))) {
        return -1;
    }

    return 0;
}