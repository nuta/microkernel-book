#include "ethernet.h"
#include "arp.h"
#include "device.h"
#include "ipv4.h"
#include "main.h"
#include "mbuf.h"
#include <libs/common/endian.h>
#include <libs/common/print.h>
#include <libs/common/string.h>

// イーサーネットフレームを送信する。
void ethernet_transmit(enum ether_type type, ipv4addr_t dst, mbuf_t payload) {
    // パケットをどこに送るかを決定する。
    ipv4addr_t next_hop = device_get_next_hop(dst);
    if (device_get_next_hop(dst) == IPV4_ADDR_UNSPECIFIED) {
        WARN("ethernet_transmit: no route to %pI4", dst);
        mbuf_delete(payload);
        return;
    }

    // 宛先のMACアドレスを取得する。
    macaddr_t dst_macaddr;
    if (!arp_resolve(next_hop, &dst_macaddr)) {
        // 宛先のMACアドレスがARPテーブルに見つからなかったため、即座に送信できない。
        // ARPテーブルの応答待ちキューにパケットを挿入し、ARPリクエストを送信して処理を
        // 終える。
        arp_enqueue(type, next_hop, payload);
        arp_request(next_hop);
        return;
    }

    // 宛先MACアドレスが見つかったので、パケットを送信する。
    struct ethernet_header header;
    memcpy(header.dst, dst_macaddr, MACADDR_LEN);
    memcpy(header.src, device_get_macaddr(), MACADDR_LEN);
    header.type = hton16(type);
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_append(pkt, payload);

    // ネットワークデバイスドライバにパケット送信を依頼する。
    callback_ethernet_transmit(pkt);
}

// イーサーネットフレームの受信処理。
void ethernet_receive(const void *pkt, size_t len) {
    mbuf_t m = mbuf_new(pkt, len);

    // イーサーネットフレームのヘッダを取り出す。
    struct ethernet_header header;
    if (mbuf_read(&m, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    uint16_t type = ntoh16(header.type);
    switch (type) {
        case ETHER_TYPE_ARP:
            // ARPパケット
            arp_receive(m);
            break;
        case ETHER_TYPE_IPV4:
            // IPv4パケット
            ipv4_receive(m);
            break;
        default:
            WARN("unknown ethernet type: %x", type);
    }
}
