#pragma once
#include "arp.h"
#include "ethernet.h"
#include "ipv4.h"
#include "mbuf.h"

// デバイス管理構造体
struct device {
    bool initialized;    // デバイスが初期化されたかどうか
    bool dhcp_enabled;   // DHCPが有効化されているかどうか
    macaddr_t macaddr;   // MACアドレス
    ipv4addr_t ipaddr;   // IPアドレス
    ipv4addr_t gateway;  // デフォルトゲートウェイ
    ipv4addr_t netmask;  // ネットマスク
};

bool device_dst_is_ours(ipv4addr_t dst);
ipv4addr_t device_get_next_hop(ipv4addr_t dst);
macaddr_t *device_get_macaddr(void);
ipv4addr_t device_get_ipaddr(void);
void device_set_ip_addrs(ipv4addr_t ipaddr, ipv4addr_t netmask,
                         ipv4addr_t gateway);
bool device_ready(void);
void device_enable_dhcp(void);
void device_init(macaddr_t *macaddr);
