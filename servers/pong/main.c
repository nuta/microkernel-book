#include <libs/common/print.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>

void main(void) {
    // pongサーバとして登録する
    ASSERT_OK(ipc_register("pong"));
    TRACE("ready");

    // メインループ
    while (true) {
        struct message m;
        ASSERT_OK(ipc_recv(IPC_ANY, &m));
        switch (m.type) {
            case PING_MSG: {
                DBG("received ping message from #%d (value=%d)", m.src,
                    m.ping.value);

                m.type = PING_REPLY_MSG;
                m.ping_reply.value = 42;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                WARN("unhandled message: %s (%x)", msgtype2str(m.type), m.type);
                break;
        }
    }
}
