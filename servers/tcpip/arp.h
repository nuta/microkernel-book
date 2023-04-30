#pragma once
#include "ethernet.h"
#include "ipv4.h"
#include "mbuf.h"
#include <libs/common/list.h>

// ARPテーブルのエントリ数
#define ARP_ENTRIES_MAX 128

// ARPテーブルのエントリ
struct arp_entry {
    bool in_use;        // 使用中かどうか
    bool resolved;      // MACアドレスが解決済みかどうか
    ipv4addr_t ipaddr;  // IPv4アドレス (ネクストホップ)
    macaddr_t macaddr;  // MACアドレス (resolved == trueのときのみ有効)
    list_t queue;       // ARP応答待ちパケットリスト
    int time_accessed;  // 最後にアクセスした時刻
};

// ARPテーブル
struct arp_table {
    struct arp_entry entries[ARP_ENTRIES_MAX];
};

// ARP応答待ちパケットリストのエントリ
struct arp_queue_entry {
    list_elem_t next;      // リストの次の要素へのポインタ
    ipv4addr_t dst;        // 宛先IPv4アドレス　(ネクストホップではなくIPヘッダの宛先)
    enum ether_type type;  // ペイロードの種類
    mbuf_t payload;        // ペイロード
};

// ARPパケットの種類
enum arp_opcode {
    ARP_OP_REQUEST = 1,  // ARP要求
    ARP_OP_REPLY = 2,    // ARP応答
};

// ARPパケット
struct arp_packet {
    uint16_t hw_type;      // ハードウェアの種類　(イーサーネットの場合は1)
    uint16_t proto_type;   // プロトコルの種類 (IPv4の場合は0x0800)
    uint8_t hw_size;       // ハードウェアアドレスの長さ (MACアドレスの場合は6)
    uint8_t proto_size;    // プロトコルアドレスの長さ (IPv4アドレスの場合は4)
    uint16_t opcode;       // ARPパケットの種類 (ARP_OP_REQUEST または ARP_OP_REPLY)
    macaddr_t sender;      // 送信元MACアドレス
    uint32_t sender_addr;  // 送信元IPv4アドレス
    macaddr_t target;      // 宛先MACアドレス
    uint32_t target_addr;  // 宛先IPv4アドレス
} __packed;

bool arp_resolve(ipv4addr_t ipaddr, macaddr_t *macaddr);
void arp_enqueue(enum ether_type type, ipv4addr_t dst, mbuf_t payload);
void arp_register_macaddr(ipv4addr_t ipaddr, macaddr_t macaddr);
void arp_request(ipv4addr_t addr);
void arp_receive(mbuf_t pkt);
