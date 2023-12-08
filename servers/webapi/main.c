#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/common/vprintf.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>

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

static void process(int sock, uint8_t *data, size_t len) {
    // todo: fix this
    static char buf[] = "HTTP/1.1 200 OK\r\nConnection: close\rContent-Length: 109\r\n\r\n" INDEX_HTML;

    struct message m;
    m.type = TCPIP_WRITE_MSG;
    m.tcpip_write.sock = sock;
    memcpy(m.tcpip_write.data, buf, sizeof(buf));
    m.tcpip_write.data_len = strlen(buf);
    ASSERT_OK(ipc_call(tcpip_server, &m));
}

void main(void) {
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
                // close socket
                m.type = TCPIP_CLOSE_MSG;
                m.tcpip_close.sock = m.tcpip_close.sock;
                ipc_call(tcpip_server, &m);
                return;
            }

            case TCPIP_DATA_MSG: {
                int sock = m.tcpip_data.sock;
                m.type = TCPIP_READ_MSG;
                m.tcpip_read.sock = sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));
                process(sock, m.tcpip_read_reply.data, m.tcpip_read_reply.data_len);
                break;
            }

            default:
                WARN("unknown message type: %s from %d", msgtype2str(m.type),
                     m.src);
        }
    }
}
