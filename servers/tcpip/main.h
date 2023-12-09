#pragma once
#include "device.h"
#include "tcp.h"
#include <libs/common/list.h>
#include <libs/common/types.h>
#include <libs/user/ipc.h>

#define TIMER_INTERVAL 100
#define SOCKETS_MAX    256

// ソケット管理構造体
struct socket {
    bool used;                // 使用中か
    task_t task;              // 所有するタスク
    int fd;                   // ソケットID
    struct tcp_pcb *tcp_pcb;  // TCPコントロールブロック
};

void callback_ethernet_transmit(mbuf_t pkt);
void callback_tcp_data(struct tcp_pcb *sock);
void callback_tcp_rst(struct tcp_pcb *sock);
void callback_tcp_closed(struct tcp_pcb *sock);
void callback_dns_got_answer(ipv4addr_t addr, void *arg);
