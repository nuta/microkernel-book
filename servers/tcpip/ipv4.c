#include "ipv4.h"
#include "checksum.h"
#include "device.h"
#include "ethernet.h"
#include "tcp.h"
#include "udp.h"
#include <libs/common/endian.h>
#include <libs/common/print.h>
#include <libs/common/string.h>

// IPv4パケットの送信処理
void ipv4_transmit(ipv4addr_t dst, uint8_t proto, mbuf_t payload) {
    // IPv4ヘッダを構築
    struct ipv4_header header;
    memset(&header, 0, sizeof(header));
    size_t total_len = sizeof(header) + mbuf_len(payload);
    header.ver_ihl = 0x45;                          // ヘッダ長
    header.len = hton16(total_len);                 // 合計の長さ
    header.ttl = DEFAULT_TTL;                       // TTL
    header.proto = proto;                           // 上層のプロトコル
    header.dst_addr = hton32(dst);                  // 宛先IPv4アドレス
    header.src_addr = hton32(device_get_ipaddr());  // 送信元IPv4アドレス

    // チェックサムを計算してセット
    checksum_t checksum;
    checksum_init(&checksum);
    checksum_update(&checksum, &header, sizeof(header));
    header.checksum = checksum_finish(&checksum);

    // IPv4ヘッダを先頭に付けてイーサーネットの送信処理に回す
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_append(pkt, payload);
    ethernet_transmit(ETHER_TYPE_IPV4, dst, pkt);
}

// IPv4パケットの受信処理
void ipv4_receive(mbuf_t pkt) {
    // IPv4ヘッダを読み込む
    struct ipv4_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) < sizeof(header)) {
        return;
    }

    // ヘッダサイズを取得して、ヘッダの長さ分だけパケットを捨てる。これでpktはIPv4パケットの
    // ペイロードを指すようになる。
    size_t header_len = (header.ver_ihl & 0x0f) * 4;
    if (header_len > sizeof(header)) {
        mbuf_discard(&pkt, header_len - sizeof(header));
    }

    // 宛先が自分でなければ無視
    ipv4addr_t dst = ntoh32(header.dst_addr);
    if (!device_dst_is_ours(dst)) {
        return;
    }

    // 変な長さのパケットは無視
    if (ntoh16(header.len) < header_len) {
        return;
    }

    // イーサーネットフレームのペイロードは最小長制約のためにパディングされる可能性があるので、
    // IPv4ヘッダに記載されているペイロードの長さに合わせてパケットを切り詰める。こうしないと
    // TCPのペイロード長が正しく計算されない (TCPヘッダにはペイロード長が記載されていない)。
    uint16_t payload_len = ntoh16(header.len) - header_len;
    mbuf_truncate(pkt, payload_len);
    if (mbuf_len(pkt) != payload_len) {
        // 変な長さのパケットは無視
        return;
    }

    // パケットの種類に応じて処理を振り分ける
    ipv4addr_t src = ntoh32(header.src_addr);
    switch (header.proto) {
        case IPV4_PROTO_UDP:
            udp_receive(src, pkt);
            break;
        case IPV4_PROTO_TCP:
            tcp_receive(dst, src, pkt);
            break;
        default:
            WARN("unknown ip proto type: %x", header.proto);
    }
}
