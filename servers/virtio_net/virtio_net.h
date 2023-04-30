#pragma once
#include <libs/common/list.h>
#include <libs/common/message.h>
#include <libs/common/types.h>

#define NUM_TX_BUFFERS             128
#define NUM_RX_BUFFERS             128
#define VIRTIO_NET_MAX_PACKET_SIZE 1514

#define VIRTIO_NET_F_MAC       (1 << 5)
#define VIRTIO_NET_F_MRG_RXBUF (1 << 15)
#define VIRTIO_NET_F_STATUS    (1 << 16)
#define VIRTIO_NET_QUEUE_RX    0
#define VIRTIO_NET_QUEUE_TX    1

// デバイス固有のコンフィグ領域 (MMIO)
struct virtio_net_config {
    uint8_t macaddr[6];
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
} __packed;

// 処理要求のヘッダ
#define VIRTIO_NET_HDR_GSO_NONE 0
struct virtio_net_header {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t hdr_len;
    uint16_t gso_size;
    uint16_t checksum_start;
    uint16_t checksum_offset;
} __packed;

// virtio-netデバイスへの処理要求
struct virtio_net_req {
    struct virtio_net_header header;
    uint8_t payload[VIRTIO_NET_MAX_PACKET_SIZE];
} __packed;
