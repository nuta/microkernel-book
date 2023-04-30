#include "device.h"
#include "dhcp.h"
#include "ipv4.h"
#include <libs/common/print.h>
#include <libs/common/string.h>

// ネットワークデバイス。現在は1つだけのデバイスしかサポートしていない。
static struct device device;

// 2つのIPv4アドレスが、与えられたネットマスクで同じネットワークに属するかどうかを返す。
static inline bool ipaddr_equals_in_netmask(ipv4addr_t a, ipv4addr_t b,
                                            ipv4addr_t netmask) {
    return (a & netmask) == (b & netmask);
}

// 与えられた宛先IPv4アドレスへのパケットを受信すべきかどうかを返す。
bool device_dst_is_ours(ipv4addr_t dst) {
    ASSERT(device.initialized);

    return dst == device.ipaddr || dst == IPV4_ADDR_BROADCAST;
}

// 与えられた宛先IPv4アドレスへのパケットをどこに送るべきかを返す。
ipv4addr_t device_get_next_hop(ipv4addr_t dst) {
    ASSERT(device.initialized);

    if (dst == IPV4_ADDR_BROADCAST) {
        // ブロードキャストアドレスへのパケットは、ブロードキャストアドレスへ送る
        return dst;
    } else if (ipaddr_equals_in_netmask(device.ipaddr, dst, device.netmask)) {
        // 同じネットワーク内の宛先アドレスへのパケットは、そのまま送る
        return dst;
    } else {
        // それ以外の宛先アドレスへのパケットは、デフォルトゲートウェイへ送る
        ASSERT(device.gateway != IPV4_ADDR_UNSPECIFIED);
        return device.gateway;
    }
}

// デバイスのMACアドレスを返す。
macaddr_t *device_get_macaddr(void) {
    ASSERT(device.initialized);

    return &device.macaddr;
}

// デバイスのIPアドレスを返す。
ipv4addr_t device_get_ipaddr(void) {
    ASSERT(device.initialized);

    return device.ipaddr;
}

// デバイスのIPアドレス、ネットマスク、デフォルトゲートウェイを設定する。
void device_set_ip_addrs(ipv4addr_t ipaddr, ipv4addr_t netmask,
                         ipv4addr_t gateway) {
    ASSERT(device.initialized);

    device.ipaddr = ipaddr;
    device.netmask = netmask;
    device.gateway = gateway;
}

// デバイスが利用可能状態かどうかを返す。
bool device_ready(void) {
    return device.initialized && device.ipaddr != IPV4_ADDR_UNSPECIFIED;
}

// デバイスのDHCPクライアントを有効化し、DHCP DISCOVERを送信する。
void device_enable_dhcp(void) {
    device.dhcp_enabled = true;
    dhcp_transmit(DHCP_TYPE_DISCOVER, IPV4_ADDR_UNSPECIFIED);
}

// デバイスの初期化を行う。
void device_init(macaddr_t *macaddr) {
    device.initialized = true;
    device.dhcp_enabled = false;
    device.ipaddr = 0;
    device.netmask = 0;
    device.gateway = 0;
    memcpy(device.macaddr, macaddr, MACADDR_LEN);
}
