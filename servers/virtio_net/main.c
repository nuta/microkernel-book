#include "virtio_net.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/dmabuf.h>
#include <libs/user/driver.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>
#include <libs/user/virtio/virtio_mmio.h>

static task_t tcpip_server;            // TCP/IPサーバのタスクID
static struct virtio_mmio device;      // virtioデバイスの管理構造体
static struct virtio_virtq *rx_virtq;  // 受信パケット用virtqueue
static struct virtio_virtq *tx_virtq;  // 送信パケット用virtqueue
static dmabuf_t rx_dmabuf;             // 受信パケット用virtqueueで使われるバッファ
static dmabuf_t tx_dmabuf;             // 送信パケット用virtqueueで使われるバッファ

// MACアドレスを読み込む
static void read_macaddr(uint8_t *macaddr) {
    offset_t base = offsetof(struct virtio_net_config, macaddr);
    for (int i = 0; i < 6; i++) {
        macaddr[i] = virtio_read_device_config8(&device, base + i);
    }
}

// パケットを送信する
static error_t transmit(const void *payload, size_t len) {
    if (len > VIRTIO_NET_MAX_PACKET_SIZE) {
        return ERR_TOO_LARGE;
    }

    // 処理要求用のバッファを割り当てる
    struct virtio_net_req *req;
    paddr_t paddr;
    if ((req = dmabuf_alloc(tx_dmabuf, &paddr)) == NULL) {
        WARN("no free TX buffers");
        return ERR_TRY_AGAIN;
    }

    // 処理要求を作成する
    req->header.flags = 0;
    req->header.gso_type = VIRTIO_NET_HDR_GSO_NONE;
    req->header.gso_size = 0;
    req->header.checksum_start = 0;
    req->header.checksum_offset = 0;
    memcpy((uint8_t *) &req->payload, payload, len);

    // ディスクリプタチェーンを作成する
    struct virtio_chain_entry chain[1];
    chain[0].addr = paddr;
    chain[0].len = sizeof(struct virtio_net_header) + len;
    chain[0].device_writable = false;

    // virtqueueにディスクリプタチェーンを追加する
    int index_or_err = virtq_push(tx_virtq, chain, 1);
    if (IS_ERROR(index_or_err)) {
        return index_or_err;
    }

    // デバイスに通知する
    virtq_notify(&device, tx_virtq);
    return OK;
}

// 割り込みハンドラ
static void irq_handler(void) {
    // 割り込みを受信したことをデバイスに通知する
    uint8_t status = virtio_read_interrupt_status(&device);
    virtio_ack_interrupt(&device, status);

    // 割り込みの原因: デバイスがvirtqueueを更新した
    if (status & VIRTIO_ISR_STATUS_QUEUE) {
        // 送信済みパケットを見ていくループ
        struct virtio_chain_entry chain[1];
        size_t total_len;
        while (virtq_pop(tx_virtq, chain, 1, &total_len) > 0) {
            // 送信用に割り当てたバッファを解放する
            dmabuf_free(tx_dmabuf, chain[0].addr);
        }

        // 受信済みパケットを見ていくループ
        while (virtq_pop(rx_virtq, chain, 1, &total_len) > 0) {
            // ディスクリプタの物理アドレスから対応する仮想アドレスを得る
            struct virtio_net_req *req = dmabuf_p2v(rx_dmabuf, chain[0].addr);

            // TCP/IPサーバにパケットを送信する
            struct message m;
            m.type = NET_RECV_MSG;
            memcpy(m.net_recv.payload, &req->payload, total_len);
            m.net_recv.payload_len = total_len;
            OOPS_OK(ipc_send(tcpip_server, &m));

            // 受信したメモリバッファを再度キューに戻す
            virtq_push(rx_virtq, chain, 1);
        }

        // 受信キューに再挿入したのでデバイスに通知する
        virtq_notify(&device, rx_virtq);
    }
}

// デバイスを初期化する
static void init_device(void) {
    // virtioデバイスを初期化する。
    ASSERT_OK(virtio_init(&device, VIRTIO_NET_PADDR, 2));

    // デバイスの機能を有効化する。特に必要な機能はないので、デバイスが提案した機能をそのまま
    // 有効化する。
    uint64_t features = virtio_read_device_features(&device);
    ASSERT_OK(virtio_negotiate_feature(&device, features));

    // デバイスを有効化する。
    ASSERT_OK(virtio_enable(&device));

    // virtqueueへのポインタを取得する。
    rx_virtq = virtq_get(&device, 0);
    tx_virtq = virtq_get(&device, 1);

    // 処理要求用のDMAバッファを割り当てる。
    tx_dmabuf = dmabuf_create(sizeof(struct virtio_net_req), NUM_TX_BUFFERS);
    rx_dmabuf = dmabuf_create(sizeof(struct virtio_net_req), NUM_RX_BUFFERS);
    ASSERT(tx_dmabuf != NULL);
    ASSERT(rx_dmabuf != NULL);

    // 受信用のvirtqueueを受信用メモリバッファで埋める。
    for (int i = 0; i < NUM_RX_BUFFERS; i++) {
        paddr_t paddr;
        if (!dmabuf_alloc(rx_dmabuf, &paddr)) {
            PANIC("failed to allocate a RX buffer");
        }

        struct virtio_chain_entry chain[1];
        chain[0].addr = paddr;
        chain[0].len = sizeof(struct virtio_net_req);
        chain[0].device_writable = true;
        int desc_index = virtq_push(rx_virtq, chain, 1);
        ASSERT_OK(desc_index);
    }

    // 割り込みを受信するように設定する。
    ASSERT_OK(sys_irq_listen(VIRTIO_NET_IRQ));
}

void main(void) {
    // デバイスを初期化する
    init_device();

    // デバイスのMACアドレスを読み込む
    uint8_t macaddr[6];
    read_macaddr(macaddr);
    INFO("MAC address = %02x:%02x:%02x:%02x:%02x:%02x", macaddr[0], macaddr[1],
         macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

    // ネットワークデバイスとして登録する
    ASSERT_OK(ipc_register("net_device"));
    TRACE("ready");

    // メインループ
    while (true) {
        struct message m;
        ASSERT_OK(ipc_recv(IPC_ANY, &m));
        switch (m.type) {
            // 割り込み処理
            case NOTIFY_IRQ_MSG:
                irq_handler();
                break;
            // ネットワークデバイスを開く
            case NET_OPEN_MSG: {
                tcpip_server = m.src;  // 受信したパケットの送信先
                m.type = NET_OPEN_REPLY_MSG;
                memcpy(m.net_open_reply.macaddr, macaddr,
                       sizeof(m.net_open_reply.macaddr));
                ipc_reply(m.src, &m);
                break;
            }
            // パケットを送信
            case NET_SEND_MSG: {
                OOPS_OK(transmit(m.net_send.payload, m.net_send.payload_len));
                break;
            }
            default:
                WARN("unhandled message: %s (%x)", msgtype2str(m.type), m.type);
                break;
        }
    }
}
