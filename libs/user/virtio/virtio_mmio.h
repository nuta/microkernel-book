// MMIOベースのVirtioデバイスドライバライブラリ
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1650002
#pragma once
#include "virtio.h"

// マジックナンバー。VIRTIO_REG_MAGIC_VALUEレジスタを読み込むとこの値が返る。
#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976

// デバイスレジスタ
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-1770004
#define VIRTIO_REG_MAGIC_VALUE         0x00
#define VIRTIO_REG_VERSION             0x04
#define VIRTIO_REG_DEVICE_ID           0x08
#define VIRTIO_REG_VENDOR_ID           0x0c
#define VIRTIO_REG_DEVICE_FEATURES     0x10
#define VIRTIO_REG_DEVICE_FEATURES_SEL 0x14
#define VIRTIO_REG_DRIVER_FEATURES     0x20
#define VIRTIO_REG_DRIVER_FEATURES_SEL 0x24
#define VIRTIO_REG_QUEUE_SEL           0x30
#define VIRTIO_REG_QUEUE_NUM_MAX       0x34
#define VIRTIO_REG_QUEUE_NUM           0x38
#define VIRTIO_REG_QUEUE_ALIGN         0x3c
#define VIRTIO_REG_QUEUE_PFN           0x40
#define VIRTIO_REG_QUEUE_READY         0x44
#define VIRTIO_REG_QUEUE_NOTIFY        0x50
#define VIRTIO_REG_INTERRUPT_STATUS    0x60
#define VIRTIO_REG_INTERRUPT_ACK       0x64
#define VIRTIO_REG_DEVICE_STATUS       0x70
#define VIRTIO_REG_DEVICE_CONFIG       0x100

// 割り込み状態レジスタ (VIRTIO_REG_INTERRUPT_STATUS) のビット。セットされていると
// usedリングにデバイスが処理したディスクリプタが追加されていることを意味する。
#define VIRTIO_ISR_STATUS_QUEUE (1 << 0)

// MMIOベースのvirtioデバイスを管理する構造体
struct virtio_mmio {
    uaddr_t base;
    unsigned num_queues;
    struct virtio_virtq *virtqs;
};

error_t virtio_init(struct virtio_mmio *dev, paddr_t base_paddr,
                    unsigned num_queues);
uint64_t virtio_read_device_features(struct virtio_mmio *dev);
uint8_t virtio_read_device_config8(struct virtio_mmio *dev, offset_t offset);
error_t virtio_negotiate_feature(struct virtio_mmio *dev, uint64_t features);
error_t virtio_enable(struct virtio_mmio *dev);
uint32_t virtio_read_interrupt_status(struct virtio_mmio *dev);
void virtio_ack_interrupt(struct virtio_mmio *dev, uint32_t status);
struct virtio_virtq *virtq_get(struct virtio_mmio *dev, unsigned index);
uint32_t virtq_num_descs(struct virtio_virtq *vq);
void virtq_notify(struct virtio_mmio *dev, struct virtio_virtq *vq);
int virtq_push(struct virtio_virtq *vq, struct virtio_chain_entry *chain,
               int n);
bool virtq_is_empty(struct virtio_virtq *vq);
int virtq_pop(struct virtio_virtq *vq, struct virtio_chain_entry *chain, int n,
              size_t *total_len);
