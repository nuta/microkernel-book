#include <libs/wasm/string.h>
#include <libs/wasm/ipc.h>

static task_t tcpip_server;

#define INDEX_HTML                                                              \
    "<!DOCTYPE html>"                                                           \
    "<head>"                                                                    \
    "<html>"                                                                    \
    "   <meta charset=\"utf-8\">"                                               \
    "</head>"                                                                   \
    "<body>"                                                                    \
    "<h1>Hello from WASM OS!</h1>"                                              \
    "</body>"                                                                   \
    "</html>\n"

#define ASSERT_OK(expr)                                                         \
    do {                                                                        \
        __typeof__(expr) __expr = (expr);                                       \
        if (IS_ERROR(__expr)) {                                                 \
            goto fail;                                                          \
        }                                                                       \
    } while(0)

__attribute__((export_name("main")))
int main(void) {
    // find tcpip server
    tcpip_server = ipc_lookup("tcpip");

    // send tcp_listen message
    struct message m;
    m.type = TCPIP_LISTEN_MSG;
    m.tcpip_listen.listen_port = 80;
    ASSERT_OK(ipc_call(tcpip_server, &m));

    while(true) {
        ASSERT_OK(ipc_recv(IPC_ANY, &m));
        switch (m.type) {
            case TCPIP_CLOSED_MSG: {
                // destroy socket
                m.type = TCPIP_DESTROY_MSG;
                m.tcpip_destroy.sock = m.tcpip_closed.sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));
                break;
            }
            case TCPIP_DATA_MSG: {
                int sock = m.tcpip_data.sock;

                // read request
                m.type = TCPIP_READ_MSG;
                m.tcpip_read.sock = sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));

                // response
                static char buf[] = "HTTP/1.1 200 OK\r\nConnection: close\rContent-Length: 109\r\n\r\n" INDEX_HTML;

                m.type = TCPIP_WRITE_MSG;
                m.tcpip_write.sock = sock;
                memcpy(m.tcpip_write.data, buf, sizeof(buf));
                m.tcpip_write.data_len = strlen(buf);
                ASSERT_OK(ipc_call(tcpip_server, &m));
                break;
            }

            default:
                goto fail;
        }
    }

    fail:
        return -1;
}