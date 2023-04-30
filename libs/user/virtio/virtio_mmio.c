#include <libs/common/endian.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/driver.h>
#include <libs/user/malloc.h>
#include <libs/user/mmio.h>
#include <libs/user/syscall.h>
#include <libs/user/virtio/virtio_mmio.h>

// デバイスステータスレジスタを読み込む
static uint32_t read_device_status(struct virtio_mmio *dev) {
    return mmio_read32le(dev->base + VIRTIO_REG_DEVICE_STATUS);
}

// デバイスステータスレジスタに書き込む
static void write_device_status(struct virtio_mmio *dev, uint8_t value) {
    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_STATUS, value);
}

// virtqueueの初期化をする
static void virtq_init(struct virtio_mmio *dev, unsigned index) {
    // virtqueueを選択する
    mmio_write32le(dev->base + VIRTIO_REG_QUEUE_SEL, index);

    // virtqueueのディスクリプタ数を取得する
    uint32_t num_descs_max =
        mmio_read32le(dev->base + VIRTIO_REG_QUEUE_NUM_MAX);
    ASSERT(num_descs_max > 0);

    // virtqueueのディスクリプタ数を設定する
    uint32_t num_descs = MIN(num_descs_max, 512);
    mmio_write32le(dev->base + VIRTIO_REG_QUEUE_NUM, num_descs);

    // virtqueue上の各領域のオフセットを計算する
    offset_t avail_ring_off = sizeof(struct virtq_desc) * num_descs;
    size_t avail_ring_size = sizeof(uint16_t) * (3 + num_descs);
    offset_t used_ring_off =
        ALIGN_UP(avail_ring_off + avail_ring_size, PAGE_SIZE);
    size_t used_ring_size =
        sizeof(uint16_t) * 3 + sizeof(struct virtq_used_elem) * num_descs;

    // 必要な物理メモリ領域のサイズを計算する
    size_t virtq_size = used_ring_off + ALIGN_UP(used_ring_size, PAGE_SIZE);

    // virtqueue用の物理メモリ領域を確保する
    uaddr_t virtq_uaddr;
    paddr_t virtq_paddr;
    ASSERT_OK(driver_alloc_pages(virtq_size, PAGE_READABLE | PAGE_WRITABLE,
                                 &virtq_uaddr, &virtq_paddr));

    // virtqueueの管理構造体を初期化する
    struct virtio_virtq *vq = &dev->virtqs[index];
    vq->index = index;
    vq->num_descs = num_descs;
    vq->last_used_index = 0;
    vq->descs = (struct virtq_desc *) virtq_uaddr;
    vq->avail = (struct virtq_avail *) (virtq_uaddr + avail_ring_off);
    vq->used = (struct virtq_used *) (virtq_uaddr + used_ring_off);

    // 全てのディスクリプタをつなぎ合わせて空きリストを作る
    vq->free_head = 0;
    vq->num_free_descs = num_descs;
    for (size_t i = 0; i < num_descs; i++) {
        vq->descs[i].next = (i + 1 == num_descs) ? 0 : i + 1;
    }

    // virtqueueの物理メモリ領域をデバイスに設定する
    mmio_write32le(dev->base + VIRTIO_REG_QUEUE_ALIGN, 0);
    mmio_write32le(dev->base + VIRTIO_REG_QUEUE_PFN, virtq_paddr);

    // virtqueueを有効化する
    mmio_write32le(dev->base + VIRTIO_REG_QUEUE_READY, 1);
}

// index番目のvirtqueueを取得する
struct virtio_virtq *virtq_get(struct virtio_mmio *dev, unsigned index) {
    DEBUG_ASSERT(index < dev->num_queues);
    return &dev->virtqs[index];
}

// virtqueueのディスクリプタ数を取得する
uint32_t virtq_num_descs(struct virtio_virtq *vq) {
    return vq->num_descs;
}

// デバイスにvirtqueueにディスクリプタが追加されたことを通知する
void virtq_notify(struct virtio_mmio *dev, struct virtio_virtq *vq) {
    // ディスクリプタなどメモリへの書き込みが完了し、デバイス側から書き込みが見えることを保証する
    full_memory_barrier();

    mmio_write32le(dev->base + VIRTIO_REG_QUEUE_NOTIFY, vq->index);
}

// ディスクリプタチェーンをavailableリングに追加し、成功すれば先頭ディスクリプタのディスクリプタ
// テーブル上のインデックスを返す。また、引数chainの各ディスクリプタエントリのdesc_index
// フィールドにもインデックスが書き込まれる。
//
// 注意: 追加したディスクリプタチェーンを処理するためには、virtq_notify関数を呼び出す必要がある。
int virtq_push(struct virtio_virtq *vq, struct virtio_chain_entry *chain,
               int n) {
    DEBUG_ASSERT(n > 0);
    if (n > vq->num_free_descs) {
        // 空きディスクリプタが足りない。usedリングの中にある処理済みディスクリプタチェーンを
        // フリーリストに戻す。
        while (vq->last_used_index != vq->used->index) {
            struct virtq_used_elem *used_elem =
                &vq->used->ring[vq->last_used_index % vq->num_descs];

            // ディスクリプタチェーンの各ディスクリプタ (desc) をフリーリストに戻していく
            int num_freed = 0;
            int next_desc_index = used_elem->id;
            while (true) {
                struct virtq_desc *desc = &vq->descs[next_desc_index];
                num_freed++;

                if ((desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
                    // チェーンの最後のディスクリプタ
                    break;
                }

                next_desc_index = desc->next;
            }

            // フリーリストの先頭に追加する
            vq->free_head = used_elem->id;
            vq->num_free_descs += num_freed;
            vq->last_used_index++;
        }
    }

    // 空きディスクリプタがまだ足りないので、エラーを返す
    if (n > vq->num_free_descs) {
        return ERR_NO_MEMORY;
    }

    // フリーリストからn個のディスクリプタを取り出して、ディスクリプタを構築する
    int head_index = vq->free_head;
    int desc_index = head_index;
    struct virtq_desc *desc = NULL;
    for (int i = 0; i < n; i++) {
        struct virtio_chain_entry *e = &chain[i];
        e->desc_index = desc_index;
        desc = &vq->descs[desc_index];
        desc->addr = into_le64(e->addr);
        desc->len = into_le32(e->len);

        if (i + 1 < n) {
            // チェーンの途中のディスクリプタ
            desc->flags = VIRTQ_DESC_F_NEXT;  // 次のディスクリプタがある
        } else {
            // チェーンの最後のディスクリプタ
            vq->free_head = desc->next;  // 次のディスクリプタを解放する
            desc->flags = 0;             // 次のディスクリプタはない
            desc->next = 0;              // 使わない
        }

        if (e->device_writable) {
            desc->flags |= VIRTQ_DESC_F_WRITE;  // デバイスから書き込み専用　
        }

        desc_index = desc->next;  // 次の空きディスクリプタのインデックス
        vq->num_free_descs--;
    }

    // availableリングにディスクリプタチェーンの先頭ディスクリプタのインデックスを追加する
    vq->avail->ring[vq->avail->index % vq->num_descs] = head_index;
    full_memory_barrier();
    // availableリングのインデックスを更新する
    vq->avail->index++;
    return head_index;
}

// デバイスドライバが処理すべきディスクリプタチェーンがusedリングにあるかどうかを返す
bool virtq_is_empty(struct virtio_virtq *vq) {
    return vq->last_used_index == mmio_read16le((uaddr_t) &vq->used->index);
}

// デバイスが処理したディスクリプタチェーンを取り出す。チェーンに含まれるディスクリプタの数を返し、
// `chain`に取り出したディスクリプタを格納する。
//
// 処理済みのディスクリプタチェーンがない場合は、ERR_EMPTYを返す。
int virtq_pop(struct virtio_virtq *vq, struct virtio_chain_entry *chain, int n,
              size_t *total_len) {
    if (virtq_is_empty(vq)) {
        return ERR_EMPTY;
    }

    // usedリングの先頭のディスクリプタチェーンを取り出す
    struct virtq_used_elem *used_elem =
        &vq->used->ring[vq->last_used_index % vq->num_descs];

    // ディスクリプタチェーン中の各ディスクリプタを処理する
    int next_desc_index = used_elem->id;  // チェーンの先頭ディスクリプタ
    struct virtq_desc *desc = NULL;
    int num_popped = 0;
    while (num_popped < n) {
        desc = &vq->descs[next_desc_index];
        chain[num_popped].desc_index = next_desc_index;
        chain[num_popped].addr = desc->addr;
        chain[num_popped].len = desc->len;
        chain[num_popped].device_writable =
            (desc->flags & VIRTQ_DESC_F_WRITE) != 0;
        num_popped++;

        bool has_next = (desc->flags & VIRTQ_DESC_F_NEXT) != 0;
        if (!has_next) {
            break;
        }

        if (num_popped >= n && has_next) {
            // 引数 n で指定した数より多くのディスクリプタを処理しようとしている
            return ERR_NO_MEMORY;
        }

        next_desc_index = desc->next;
    }

    // 処理したディスクリプタたちをフリーリストに戻す
    DEBUG_ASSERT(desc != NULL);
    desc->next = vq->free_head;
    vq->free_head = used_elem->id;
    vq->num_free_descs += num_popped;

    *total_len = used_elem->len;
    vq->last_used_index++;
    return num_popped;
}

// デバイスコンフィグを1バイト読み込む
uint8_t virtio_read_device_config8(struct virtio_mmio *dev, offset_t offset) {
    return mmio_read8(dev->base + VIRTIO_REG_DEVICE_CONFIG + offset);
}

// 割り込みステータスレジスタを読み込む
uint32_t virtio_read_interrupt_status(struct virtio_mmio *dev) {
    return mmio_read32le(dev->base + VIRTIO_REG_INTERRUPT_STATUS);
}

// 割り込み処理を完了した旨をデバイスに通知する
void virtio_ack_interrupt(struct virtio_mmio *dev, uint32_t status) {
    mmio_write32le(dev->base + VIRTIO_REG_INTERRUPT_ACK, status);
}

// デバイスが対応してる機能を読み込む
uint64_t virtio_read_device_features(struct virtio_mmio *dev) {
    // 下位32ビットを読み込む
    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_FEATURES_SEL, 0);
    uint32_t low = mmio_read32le(dev->base + VIRTIO_REG_DEVICE_FEATURES);

    // 上位32ビットを読み込む
    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_FEATURES_SEL, 1);
    uint32_t high = mmio_read32le(dev->base + VIRTIO_REG_DEVICE_FEATURES);
    return ((uint64_t) high << 32) | low;
}

// デバイスに対して引数featuresで指定した機能を有効にする。対応していない機能がある場合は、
// ERR_NOT_SUPPORTEDを返す。
error_t virtio_negotiate_feature(struct virtio_mmio *dev, uint64_t features) {
    if ((virtio_read_device_features(dev) & features) != features) {
        return ERR_NOT_SUPPORTED;
    }

    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_FEATURES_SEL, 0);
    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_FEATURES, features);
    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_FEATURES_SEL, 1);
    mmio_write32le(dev->base + VIRTIO_REG_DEVICE_FEATURES, features >> 32);
    write_device_status(dev, read_device_status(dev) | VIRTIO_STATUS_FEAT_OK);

    if ((read_device_status(dev) & VIRTIO_STATUS_FEAT_OK) == 0) {
        return ERR_NOT_SUPPORTED;
    }

    return OK;
}

// virtioデバイスを初期化する。この後にvirtio_negotiate_feature関数でデバイスの機能を
// 有効化し、virtio_enable関数でデバイスを有効化する必要がある。
error_t virtio_init(struct virtio_mmio *dev, paddr_t base_paddr,
                    unsigned num_queues) {
    error_t err = driver_map_pages(base_paddr, PAGE_SIZE,
                                   PAGE_READABLE | PAGE_WRITABLE, &dev->base);
    if (err != OK) {
        return err;
    }

    if (mmio_read32le(dev->base + VIRTIO_REG_MAGIC_VALUE)
        != VIRTIO_MMIO_MAGIC_VALUE) {
        return ERR_NOT_SUPPORTED;
    }

    // デバイスをリセットする
    write_device_status(dev, 0);
    write_device_status(dev, read_device_status(dev) | VIRTIO_STATUS_ACK);
    write_device_status(dev, read_device_status(dev) | VIRTIO_STATUS_DRIVER);

    // 各virtqueueを初期化する
    dev->num_queues = num_queues;
    dev->virtqs = malloc(sizeof(*dev->virtqs) * num_queues);
    for (unsigned i = 0; i < num_queues; i++) {
        virtq_init(dev, i);
    }

    return OK;
}

// virtioデバイスを有効にする
error_t virtio_enable(struct virtio_mmio *dev) {
    write_device_status(dev, read_device_status(dev) | VIRTIO_STATUS_DRIVER_OK);
    return OK;
}
