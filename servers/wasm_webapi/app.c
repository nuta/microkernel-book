#include <libs/user/ipc.h>

// Library functions
// TODO: Replace with wasi-libc?
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

void *memcpy(void *dst, const void *src, size_t len) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (len-- > 0) {
        *d = *s;
        d++;
        s++;
    }
    return dst;
}

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
    // Find tcpip server
    tcpip_server = ipc_lookup("tcpip");

    // Send tcp_listen message
    struct message m;
    m.type = TCPIP_LISTEN_MSG;
    m.tcpip_listen.listen_port = 80;
    ASSERT_OK(ipc_call(tcpip_server, &m));

    while(true) {
        ASSERT_OK(ipc_recv(IPC_ANY, &m));
        switch (m.type) {
            case TCPIP_CLOSED_MSG: {
                // Destroy socket
                m.type = TCPIP_DESTROY_MSG;
                m.tcpip_destroy.sock = m.tcpip_closed.sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));
                break;
            }
            case TCPIP_DATA_MSG: {
                int sock = m.tcpip_data.sock;

                // Read request
                m.type = TCPIP_READ_MSG;
                m.tcpip_read.sock = sock;
                ASSERT_OK(ipc_call(tcpip_server, &m));

                // Response
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
