#include "http.h"
#include <libs/common/ctype.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>

static task_t tcpip_server;

static void send(int sock, const uint8_t *buf, size_t len) {
    struct message m;
    ASSERT(len < sizeof(m.tcpip_write.data));

    m.type = TCPIP_WRITE_MSG;
    m.tcpip_write.sock = sock;
    memcpy(m.tcpip_write.data, buf, len);
    m.tcpip_write.data_len = len;
    ASSERT_OK(ipc_call(tcpip_server, &m));
}

static void received(int sock, uint8_t *buf, size_t len) {
    // char *sep = strstr((char *) buf, "\r\n\r\n");
    buf[len] = '\0';
    DBG("%s", buf);
    return;
}

static error_t parse_ipaddr(const char *str, uint32_t *ip_addr) {
    char *s_orig = strdup(str);
    char *s = s_orig;
    char *part;
    for (int i = 0; i < 3; i++) {
        part = s;
        s = strchr(s, '.');
        if (!s) {
            free(s_orig);
            return ERR_INVALID_ARG;
        }

        *s++ = '\0';
        *ip_addr = (*ip_addr << 8) | atoi(part);
    }

    *s++ = '\0';
    *ip_addr = (*ip_addr << 8) | atoi(part);

    free(s_orig);
    return OK;
}

static error_t resolve_url(const char *url, uint32_t *ip_addr, uint16_t *port,
                           char **path) {
    char *s_orig = strdup(url);
    char *s = s_orig;
    if (strstr(s, "http://") == s) {
        s += 7;  // strlen("http://")
    } else {
        WARN("the url must start with http://");
        return ERR_INVALID_ARG;
    }

    // `s` now points to the beginning of hostname or IP address.
    char *host = s;
    char *sep = strchr(s, ':');
    if (sep) {
        *sep = '\0';
        char *port_str = sep + 1;  // Next to ':'.
        s = strchr(port_str, '/');
        *s++ = '\0';
        *port = atoi(port_str);
    } else {
        // The port number is not given.
        *port = 80;
        s = strchr(s, '/');
        if (s) {
            *s++ = '\0';
        }
    }

    if (isdigit(*host)) {
        if (parse_ipaddr(host, ip_addr) != OK) {
            WARN("failed to parse an ip address: '%s'", host);
            return ERR_INVALID_ARG;
        }
    } else {
        struct message m;
        m.type = TCPIP_DNS_RESOLVE_MSG;
        strcpy_safe(m.tcpip_dns_resolve.hostname,
                    sizeof(m.tcpip_dns_resolve.hostname), host);
        ASSERT_OK(ipc_call(tcpip_server, &m));

        *ip_addr = m.tcpip_dns_resolve_reply.addr;
    }

    // `s` now points to the path next to the first slash.

    *path = s;
    free(s_orig);
    return OK;
}

void http_get(const char *url) {
    tcpip_server = ipc_lookup("tcpip");

    uint32_t ip_addr;
    uint16_t port;
    char *path;
    if (resolve_url(url, &ip_addr, &port, &path) != OK) {
        return;
    }

    struct message m;
    m.type = TCPIP_CONNECT_MSG;
    m.tcpip_connect.dst_addr = ip_addr;
    m.tcpip_connect.dst_port = port;
    ASSERT_OK(ipc_call(tcpip_server, &m));
    int sock = m.tcpip_connect_reply.sock;

    int buf_len = 1024;
    char *buf = malloc(buf_len);

    char *p = buf;
    for (const char *s = "GET /"; *s; s++) {
        *p++ = *s;
    }
    for (const char *s = path; *s; s++) {
        *p++ = *s;
    }
    for (const char *s = " HTTP/1.0\r\nconnection: closed\r\n\r\n"; *s; s++) {
        *p++ = *s;
    }
    *p = '\0';

    send(sock, (uint8_t *) buf, strlen(buf));
    free(buf);

    while (1) {
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case TCPIP_CLOSED_MSG: {
                m.type = TCPIP_READ_MSG;
                m.tcpip_read.sock = m.tcpip_closed.sock;
                error_t err = ipc_call(tcpip_server, &m);

                ASSERT_OK(err);
                received(sock, m.tcpip_read_reply.data,
                         m.tcpip_read_reply.data_len);

                m.type = TCPIP_CLOSE_MSG;
                m.tcpip_close.sock = m.tcpip_close.sock;
                ipc_call(tcpip_server, &m);
                return;
            }
            case TCPIP_DATA_MSG: {
                m.type = TCPIP_READ_MSG;
                m.tcpip_read.sock = m.tcpip_data.sock;
                error_t err = ipc_call(tcpip_server, &m);

                ASSERT_OK(err);
                received(sock, m.tcpip_read_reply.data,
                         m.tcpip_read_reply.data_len);
                break;
            }
            default:
                WARN("unknown message type: %s from %d", msgtype2str(m.type),
                     m.src);
        }
    }
}
