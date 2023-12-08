#include "main.h"
#include "device.h"
#include "dhcp.h"
#include "dns.h"
#include "tcp.h"
#include "udp.h"
#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>
#include <libs/user/syscall.h>

// ネットワークデバイスドライバサーバ
static task_t net_device;
// ソケット管理構造体
static struct socket sockets[SOCKETS_MAX];

// ソケットIDを割り当てる。使えるソケットIDがなければ0を返す。
static struct socket *alloc_socket(void) {
    for (int i = 0; i < SOCKETS_MAX; i++) {
        if (!sockets[i].used) {
            sockets[i].fd = i + 1;
            sockets[i].used = true;
            return &sockets[i];
        }
    }

    return 0;
}

// ソケットIDからソケット構造体を取得する。存在しなければNULLを返す。
static struct socket *lookup_socket(task_t task, int sock) {
    if (sock < 1 || sock > SOCKETS_MAX) {
        return NULL;
    }

    struct socket *s = &sockets[sock - 1];
    if (!s->used || s->task != task) {
        return NULL;
    }

    return s;
}

// ソケットを解放する。
static void free_socket(struct socket *sock) {
    sock->used = false;
    tcp_close(sock->tcp_pcb);
}

// 任意のタスクが終了したときに呼ばれる。
static void do_task_destroyed(task_t task) {
    // そのタスクが所有するソケットをすべて解放する。
    for (int i = 0; i < SOCKETS_MAX; i++) {
        struct socket *s = &sockets[i];
        if (s->used && s->task == task) {
            free_socket(s);
        }
    }
}

// デバイスドライバへパケットを送信する。
void callback_ethernet_transmit(mbuf_t pkt) {
    struct message m;
    size_t len = mbuf_len(pkt);
    if (len > sizeof(m.net_send.payload)) {
        OOPS("too long packet: %d bytes", len);
        return;
    }

    uint8_t *payload = malloc(len);
    mbuf_read(&pkt, payload, len);
    mbuf_delete(pkt);

    m.type = NET_SEND_MSG;
    m.net_send.payload_len = len;
    memcpy(m.net_send.payload, payload, len);
    error_t err = ipc_send_async(net_device, &m);
    if (err != OK) {
        WARN("failed to send packet to driver: %s", err2str(err));
    }
}

// PCBからソケット構造体を取得する。
static struct socket *get_socket_from_pcb(struct tcp_pcb *pcb) {
    ASSERT(pcb->arg != NULL);
    return (struct socket *) pcb->arg;
}

// TCPソケットに新しいデータが届いたときに呼ばれる。
void callback_tcp_data(struct tcp_pcb *pcb) {
    struct socket *sock = get_socket_from_pcb(pcb);

    struct message m;
    m.type = TCPIP_DATA_MSG;
    m.tcpip_data.sock = sock->fd;
    ipc_send_async(sock->task, &m);
}

// TCPコネクションが閉じられたとき (パッシブクローズ) に呼ばれる。
void callback_tcp_fin(struct tcp_pcb *pcb) {
    struct socket *sock = get_socket_from_pcb(pcb);

    struct message m;
    m.type = TCPIP_CLOSED_MSG;
    m.tcpip_closed.sock = sock->fd;
    ipc_send_async(sock->task, &m);
}

// TCPコネクションがリセットされたときに呼ばれる。
void callback_tcp_rst(struct tcp_pcb *pcb) {
    struct socket *sock = get_socket_from_pcb(pcb);

    struct message m;
    m.type = TCPIP_CLOSED_MSG;
    m.tcpip_closed.sock = sock->fd;
    ipc_send_async(sock->task, &m);
}

// DNSサーバから応答が届いたときに呼ばれる。
void callback_dns_got_answer(ipv4addr_t addr, void *arg) {
    struct message m;
    m.type = TCPIP_DNS_RESOLVE_REPLY_MSG;
    m.tcpip_dns_resolve_reply.addr = addr;
    ipc_reply((task_t) arg, &m);
}

void main(void) {
    TRACE("starting...");

    // ソケットの解放漏れがないように、タスクが終了したときに呼んでもらうように登録する。
    struct message m;
    m.type = WATCH_TASKS_MSG;
    ASSERT_OK(ipc_call(VM_SERVER, &m));

    // ネットワークデバイスドライバに接続し、MACアドレスを取得する。
    net_device = ipc_lookup("net_device");
    ASSERT_OK(net_device);
    m.type = NET_OPEN_MSG;
    ASSERT_OK(ipc_call(net_device, &m));
    ASSERT(m.type == NET_OPEN_REPLY_MSG);

    // プロトコルスタックを初期化する。
    device_init(&m.net_open_reply.macaddr);
    dns_init();
    dhcp_init();
    device_enable_dhcp();

    // DHCPでIPアドレスを取得できるまでのループ。まだアプリケーションからの要求は受け付けない。
    while (!device_ready()) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NET_RECV_MSG: {
                // ネットワークデバイスからパケットが届いた。
                ethernet_receive(m.net_recv.payload, m.net_recv.payload_len);
                dhcp_receive();
                break;
            }
            case TASK_DESTROYED_MSG: {
                // まだ起動中なので解放すべきリソースがないため、単に無視する。
                break;
            }
            default:
                WARN(
                    "unexpected message type: %s (0x%x) (still initializing tcpip!)",
                    msgtype2str(m.type), m.type);
        }
    }

    // 定期的な処理をするためにタイムアウトを設定する。
    ASSERT_OK(sys_time(TIMER_INTERVAL));
    // TCP/IPサーバとしてサービス登録をする。
    ASSERT_OK(ipc_register("tcpip"));

    TRACE("ready");
    while (true) {
        // TCPの送信処理を行う。
        tcp_flush();

        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFY_TIMER_MSG: {
                // メインループを回してtcp_flush関数を呼び出し、TCPの再送処理を行う
                ASSERT_OK(sys_time(TIMER_INTERVAL));
                break;
            }
            case NET_RECV_MSG: {
                // ネットワークデバイスからパケットが届いた。
                ethernet_receive(m.net_recv.payload, m.net_recv.payload_len);
                dhcp_receive();
                dns_receive();
                break;
            }
            case TASK_DESTROYED_MSG: {
                // タスクが終了したので、関連するリソースを解放する。
                if (m.src != 1) {
                    WARN("got a message from an unexpected source: %d", m.src);
                    break;
                }
                do_task_destroyed(m.task_destroyed.task);
                break;
            }
            case TCPIP_DNS_RESOLVE_MSG: {
                char hostname[sizeof(m.tcpip_dns_resolve.hostname)];
                strcpy_safe(hostname, sizeof(hostname),
                            m.tcpip_dns_resolve.hostname);

                dns_query(hostname, (void *) m.src);
                break;
            }
            case TCPIP_LISTEN_MSG: {
                // create new socket
                struct socket *sock = alloc_socket();
                struct tcp_pcb *pcb = tcp_new(sock);

                // try to bind pcb to specified port
                error_t err = tcp_bind(
                    pcb, IPV4_ADDR_UNSPECIFIED, m.tcpip_listen.listen_port
                );

                if(err != OK) {
                    // delete socket
                    sock->used = false;
                    // reply error message
                    ipc_reply_err(m.src, err);
                    break;
                }

                // listen
                tcp_listen(pcb);

                // init socket
                sock->task = m.src;
                sock->tcp_pcb = pcb;

                // reply
                m.type = TCPIP_LISTEN_REPLY_MSG;
                m.tcpip_listen_reply.sock = sock->fd;
                ipc_send_noblock(m.src, &m);
                break;
            }
            case TCPIP_CONNECT_MSG: {
                struct socket *sock = alloc_socket();
                struct tcp_pcb *pcb = tcp_new(sock);
                error_t err = tcp_connect(pcb, m.tcpip_connect.dst_addr,
                                          m.tcpip_connect.dst_port);
                if (err != OK) {
                    sock->used = false;
                    ipc_reply_err(m.src, err);
                    break;
                }

                sock->task = m.src;
                sock->tcp_pcb = pcb;

                m.type = TCPIP_CONNECT_REPLY_MSG;
                m.tcpip_connect_reply.sock = sock->fd;
                ipc_send_noblock(m.src, &m);
                break;
            }
            case TCPIP_WRITE_MSG: {
                struct socket *sock = lookup_socket(m.src, m.tcpip_write.sock);
                if (!sock) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                tcp_write(sock->tcp_pcb, m.tcpip_write.data,
                          m.tcpip_write.data_len);

                m.type = TCPIP_WRITE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            case TCPIP_READ_MSG: {
                struct socket *sock = lookup_socket(m.src, m.tcpip_read.sock);
                if (!sock) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                m.type = TCPIP_READ_REPLY_MSG;
                m.tcpip_read_reply.data_len =
                    tcp_read(sock->tcp_pcb, m.tcpip_read_reply.data,
                             sizeof(m.tcpip_read_reply.data));

                ipc_reply(m.src, &m);
                break;
            }
            case TCPIP_CLOSE_MSG: {
                struct socket *sock = lookup_socket(m.src, m.tcpip_close.sock);
                if (!sock) {
                    ipc_reply_err(m.src, ERR_INVALID_ARG);
                    break;
                }

                free_socket(sock);

                m.type = TCPIP_CLOSE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                WARN("unknown message type: %s from %d", msgtype2str(m.type),
                     m.src);
        }
    }
}
