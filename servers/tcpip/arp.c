#include "arp.h"
#include "device.h"
#include <libs/common/endian.h>
#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/malloc.h>
#include <libs/user/syscall.h>

// ARPテーブル: IPv4アドレスとMACアドレスの対応表 (キャッシュ)
static struct arp_table table;

// ARPテーブルエントリを確保する。
static struct arp_entry *alloc_entry(void) {
    struct arp_entry *e = NULL;
    struct arp_entry *oldest = NULL;
    int oldest_time = INT_MAX;
    for (int i = 0; i < ARP_ENTRIES_MAX; i++) {
        if (table.entries[i].time_accessed < oldest_time) {
            oldest = &table.entries[i];
            oldest_time = oldest->time_accessed;
        }

        if (!table.entries[i].in_use) {
            e = &table.entries[i];
            break;
        }
    }

    if (!e) {
        // ARPテーブルが一杯なので、最近利用されていないエントリを削除する
        DEBUG_ASSERT(oldest);
        e = oldest;
    }

    e->in_use = true;
    list_init(&e->queue);
    return e;
}

// ARPテーブルからIPv4アドレスに対応するエントリを探す。
static struct arp_entry *lookup_entry(ipv4addr_t ipaddr) {
    for (int i = 0; i < ARP_ENTRIES_MAX; i++) {
        struct arp_entry *e = &table.entries[i];
        if (e->in_use && e->ipaddr == ipaddr) {
            return e;
        }
    }

    return NULL;
}

// ARPパケットを送信する。
static void arp_transmit(enum arp_opcode op, ipv4addr_t target_addr,
                         macaddr_t target) {
    struct arp_packet p;
    p.hw_type = hton16(1);          // Ethernet
    p.proto_type = hton16(0x0800);  // IPv4
    p.hw_size = MACADDR_LEN;
    p.proto_size = 4;
    p.opcode = hton16(op);
    p.sender_addr = hton32(device_get_ipaddr());
    p.target_addr = hton32(target_addr);
    memcpy(&p.sender, device_get_macaddr(), MACADDR_LEN);
    memcpy(&p.target, target, MACADDR_LEN);

    ethernet_transmit(ETHER_TYPE_ARP, IPV4_ADDR_BROADCAST,
                      mbuf_new(&p, sizeof(p)));
}

// IPv4アドレスからMACアドレスを解決する。
bool arp_resolve(ipv4addr_t ipaddr, macaddr_t *macaddr) {
    ASSERT(ipaddr != IPV4_ADDR_UNSPECIFIED);

    if (ipaddr == IPV4_ADDR_BROADCAST) {
        memcpy(macaddr, MACADDR_BROADCAST, MACADDR_LEN);
        return true;
    }

    struct arp_entry *e = lookup_entry(ipaddr);
    if (!e || !e->resolved) {
        return false;
    }

    e->time_accessed = sys_uptime();
    memcpy(macaddr, e->macaddr, MACADDR_LEN);
    return true;
}

// ARPテーブルエントリのARP応答待ちパケットリストに、パケットを新たに追加する。
//
// IPv4アドレス dst に対応するARP応答を受信した際に、この関数で追加されたパケットを送信する。
void arp_enqueue(enum ether_type type, ipv4addr_t dst, mbuf_t payload) {
    struct arp_entry *e = lookup_entry(dst);
    ASSERT(!e || !e->resolved);
    if (!e) {
        // ARPテーブルにエントリがないので、新たに作成する。
        e = alloc_entry();
        e->resolved = false;
        e->ipaddr = dst;
        e->time_accessed = sys_uptime();
    }

    // ARP応答待ちパケットリストに追加する。
    struct arp_queue_entry *qe = (struct arp_queue_entry *) malloc(sizeof(*qe));
    qe->dst = dst;
    qe->type = type;
    qe->payload = payload;
    list_elem_init(&qe->next);
    list_push_back(&e->queue, &qe->next);
}

// ARPリクエストを送信する。
void arp_request(ipv4addr_t addr) {
    arp_transmit(ARP_OP_REQUEST, addr, MACADDR_BROADCAST);
}

// ARPテーブルにMACアドレスを登録する。ARP応答が受信された際に呼び出される。
void arp_register_macaddr(ipv4addr_t ipaddr, macaddr_t macaddr) {
    struct arp_entry *e = lookup_entry(ipaddr);
    if (!e) {
        e = alloc_entry();
    }

    e->resolved = true;
    e->ipaddr = ipaddr;
    e->time_accessed = sys_uptime();
    memcpy(e->macaddr, macaddr, MACADDR_LEN);

    // ARP応答待ちパケットリストに登録されていたパケットを送信する。
    LIST_FOR_EACH (qe, &e->queue, struct arp_queue_entry, next) {
        ethernet_transmit(qe->type, qe->dst, qe->payload);
        list_remove(&qe->next);
        free(qe);
    }
}

// ARPパケットの受信処理。
void arp_receive(mbuf_t pkt) {
    struct arp_packet p;
    if (mbuf_read(&pkt, &p, sizeof(p)) != sizeof(p)) {
        return;
    }

    uint16_t opcode = ntoh16(p.opcode);
    ipv4addr_t sender_addr = ntoh32(p.sender_addr);
    ipv4addr_t target_addr = ntoh32(p.target_addr);
    switch (opcode) {
        // ARP要求
        case ARP_OP_REQUEST:
            if (device_get_ipaddr() != target_addr) {
                break;
            }

            arp_transmit(ARP_OP_REPLY, sender_addr, p.sender);
            break;
        // ARP応答
        case ARP_OP_REPLY:
            arp_register_macaddr(sender_addr, p.sender);
            break;
    }

    mbuf_delete(pkt);
}
