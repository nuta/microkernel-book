#pragma once
#include "ipv4.h"
#include "mbuf.h"

// ブロードキャストアドレス
#define MACADDR_BROADCAST ((macaddr_t){0xff, 0xff, 0xff, 0xff, 0xff, 0xff})

// MACアドレスを表す型
#define MACADDR_LEN 6
typedef uint8_t macaddr_t[MACADDR_LEN];

// イーサネットフレームの種類
enum ether_type {
    ETHER_TYPE_IPV4 = 0x0800,
    ETHER_TYPE_ARP = 0x0806,
};

// イーサーネットフレームヘッダ
struct ethernet_header {
    macaddr_t dst;  // 宛先MACアドレス
    macaddr_t src;  // 送信元MACアドレス
    uint16_t type;  // ペイロードの種類
} __packed;

void ethernet_transmit(enum ether_type type, ipv4addr_t dst, mbuf_t payload);
void ethernet_receive(const void *pkt, size_t len);
