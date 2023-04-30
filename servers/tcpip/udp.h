#pragma once
#include "ipv4.h"
#include "mbuf.h"
#include <libs/common/list.h>

// UDPヘッダ
struct udp_header {
    uint16_t src_port;  // 送信元ポート番号
    uint16_t dst_port;  // 宛先ポート番号
    uint16_t len;       // UDPペイロード長
    uint16_t checksum;  // チェックサム
} __packed;

// UDPデータグラムを表す構造体
struct udp_datagram {
    list_elem_t next;  // リストの次の要素へのポインタ
    ipv4addr_t addr;   // 送信元IPv4アドレス
    port_t port;       // 送信元ポート番号
    mbuf_t payload;    // UDPペイロード
};

// 最大UDP通信数
#define UDP_PCBS_MAX 512

// UDP通信の管理構造体
struct udp_pcb {
    list_elem_t next;  // active_socksの次の要素へのポインタ
    bool in_use;       // 使用中かどうか
    endpoint_t local;  // ソケットに紐付けられたIPアドレスとポート番号
    list_t rx;         // 受信済みデータグラムのリスト
    list_t tx;         // 送信待ちデータグラムのリスト
};

// UDPソケットを表す型。いわゆるopaqueポインタ。
typedef struct udp_pcb *udp_sock_t;

udp_sock_t udp_new(void);
void udp_close(udp_sock_t sock);
void udp_bind(udp_sock_t sock, ipv4addr_t addr, port_t port);
void udp_sendto_mbuf(udp_sock_t sock, ipv4addr_t dst, port_t dst_port,
                     mbuf_t payload);
void udp_sendto(udp_sock_t sock, ipv4addr_t dst, port_t dst_port,
                const void *data, size_t len);
mbuf_t udp_recv_mbuf(udp_sock_t sock, ipv4addr_t *src, port_t *src_port);
size_t udp_recv(udp_sock_t sock, void *buf, size_t buf_len, ipv4addr_t *src,
                port_t *src_port);
void udp_transmit(udp_sock_t sock);
void udp_receive(ipv4addr_t src, mbuf_t pkt);
void udp_init(void);
