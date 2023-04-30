#include "udp.h"
#include "checksum.h"
#include "device.h"
#include "ipv4.h"
#include <libs/common/endian.h>
#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/malloc.h>

// UDPソケット管理構造体のテーブル
static struct udp_pcb pcbs[UDP_PCBS_MAX];
// 使用中のUDPソケット管理構造体のリスト
static list_t active_pcbs = LIST_INIT(active_pcbs);

// ポート番号に対応するUDPソケット管理構造体を探す。
static struct udp_pcb *udp_lookup(port_t dst_port) {
    LIST_FOR_EACH (pcb, &active_pcbs, struct udp_pcb, next) {
        if (pcb->local.port == dst_port) {
            return pcb;
        }
    }

    return NULL;
}

// 新しいUDPソケットを割り当てる。
struct udp_pcb *udp_new(void) {
    // 未使用のUDPソケット管理構造体を探す。
    struct udp_pcb *pcb = NULL;
    for (int i = 0; i < UDP_PCBS_MAX; i++) {
        if (!pcbs[i].in_use) {
            pcb = &pcbs[i];
            break;
        }
    }

    if (!pcb) {
        return NULL;
    }

    // 管理構造体を初期化する
    pcb->in_use = true;
    pcb->local.addr = 0;
    pcb->local.port = 0;
    list_init(&pcb->rx);
    list_init(&pcb->tx);
    list_elem_init(&pcb->next);
    return pcb;
}

// UDPソケットを閉じる。
void udp_close(struct udp_pcb *pcb) {
    list_remove(&pcb->next);
    pcb->in_use = false;
}

// UDPソケットにローカルアドレスとポート番号を紐付ける。
void udp_bind(struct udp_pcb *pcb, ipv4addr_t addr, port_t port) {
    ASSERT(udp_lookup(port) == NULL);

    pcb->local.addr = addr;
    pcb->local.port = port;
    list_push_back(&active_pcbs, &pcb->next);
}

// UDPデータグラムを送信する。ペイロードとしてmbufを使う。
void udp_sendto_mbuf(struct udp_pcb *pcb, ipv4addr_t dst, port_t dst_port,
                     mbuf_t payload) {
    struct udp_datagram *dg = (struct udp_datagram *) malloc(sizeof(*dg));
    dg->addr = dst;
    dg->port = dst_port;
    dg->payload = payload;
    list_elem_init(&dg->next);
    list_push_back(&pcb->tx, &dg->next);
}

// UDPデータグラムを送信する。
void udp_sendto(struct udp_pcb *pcb, ipv4addr_t dst, port_t dst_port,
                const void *data, size_t len) {
    return udp_sendto_mbuf(pcb, dst, dst_port, mbuf_new(data, len));
}

// 受信済みのUDPデータグラムを取り出す。ペイロードをmbufで返す。
mbuf_t udp_recv_mbuf(struct udp_pcb *pcb, ipv4addr_t *src, port_t *src_port) {
    list_elem_t *e = list_pop_front(&pcb->rx);
    if (!e) {
        return NULL;
    }

    struct udp_datagram *dg = LIST_CONTAINER(e, struct udp_datagram, next);
    mbuf_t payload = dg->payload;
    *src = dg->addr;
    *src_port = dg->port;
    free(dg);
    return payload;
}

// 受信済みのUDPデータグラムを取り出す。
size_t udp_recv(struct udp_pcb *pcb, void *buf, size_t buf_len, ipv4addr_t *src,
                port_t *src_port) {
    mbuf_t payload = udp_recv_mbuf(pcb, src, src_port);
    if (!payload) {
        return 0;
    }

    size_t len = mbuf_len(payload);
    mbuf_read(&payload, buf, buf_len);
    mbuf_delete(payload);
    return len;
}

// UDPパケットの送信処理
void udp_transmit(struct udp_pcb *pcb) {
    // 送信するデータグラムを取得
    list_elem_t *e = list_pop_front(&pcb->tx);
    if (!e) {
        return;
    }

    // UDPパケットを構築
    struct udp_datagram *dg = LIST_CONTAINER(e, struct udp_datagram, next);
    struct udp_header header;
    size_t total_len = sizeof(header) + mbuf_len(dg->payload);
    header.dst_port = hton16(dg->port);         // 宛先ポート
    header.src_port = hton16(pcb->local.port);  // 送信元ポート
    header.checksum = 0;                        // チェックサム (あとで計算する)
    header.len = hton16(total_len);             // ヘッダ長 + ペイロード長

    // チェックサムを計算してセット
    checksum_t checksum;
    checksum_init(&checksum);
    checksum_update_mbuf(&checksum, dg->payload);
    checksum_update(&checksum, &header, sizeof(header));
    checksum_update_uint32(&checksum, hton32(dg->addr));
    checksum_update_uint32(&checksum, hton32(device_get_ipaddr()));
    checksum_update_uint16(&checksum, hton16(total_len));
    checksum_update_uint16(&checksum, hton16(IPV4_PROTO_UDP));
    header.checksum = checksum_finish(&checksum);

    // UDPヘッダとペイロードを新しいmbufにコピーする
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    mbuf_append(pkt, dg->payload);

    // IPv4の送信処理に回す
    ipv4_transmit(dg->addr, IPV4_PROTO_UDP, pkt);
}

// UDPパケットの受信処理
void udp_receive(ipv4addr_t src, mbuf_t pkt) {
    // UDPヘッダを読み込む
    struct udp_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    // 対応するUDPソケットを探す
    uint16_t dst_port = ntoh16(header.dst_port);
    struct udp_pcb *pcb = udp_lookup(dst_port);
    if (!pcb) {
        return;
    }

    // 受信済みデータグラムのリストに追加
    struct udp_datagram *dg = (struct udp_datagram *) malloc(sizeof(*dg));
    dg->addr = src;
    dg->port = dst_port;
    dg->payload = pkt;
    list_elem_init(&dg->next);
    list_push_back(&pcb->rx, &dg->next);
}
