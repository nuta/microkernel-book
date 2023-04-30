// Virtioデバイスドライバライブラリ
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html
#pragma once
#include <libs/common/types.h>

// デバイスステータスレジスタのビット (2.1 Device Status Field)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-110001
#define VIRTIO_STATUS_ACK       1  // OSがvirtioデバイスを認識した
#define VIRTIO_STATUS_DRIVER    2  // OSがそのデバイスの利用法を知っている
#define VIRTIO_STATUS_FEAT_OK   8  // OSがデバイスの機能を理解している
#define VIRTIO_STATUS_DRIVER_OK 4  // OSがそのデバイスを利用する準備ができた

// ディスクリプタのフラグ (2.7.5 The Virtqueue Descriptor Table)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-430005
#define VIRTQ_DESC_F_NEXT          1  // 次のディスクリプタがある
#define VIRTQ_DESC_F_WRITE         2  // (デバイスから見て) 書き込み専用
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1  // 処理を完了しても割り込みしない

/// virtqueueの管理構造体
struct virtio_virtq {
    unsigned index;             // virtqueueのインデックス
    int num_descs;              // ディスクリプタの数
    int last_used_index;        // 最後に読んだusedリングのインデックス
    int free_head;              // 空きディスクリプタのインデックス。nextフィールドで
                                // 全ての空きディスクリプタが連結されている。
    int num_free_descs;         // 空きディスクリプタの総数
    struct virtq_desc *descs;   // ディスクリプタテーブル
    struct virtq_avail *avail;  // availableリング
    struct virtq_used *used;    // usedリング
};

// ディスクリプタ (2.7.5 The Virtqueue Descriptor Table)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-430005
struct virtq_desc {
    uint64_t addr;   // DMA領域の (物理) メモリアドレス
    uint32_t len;    // DMA領域のサイズ
    uint16_t flags;  // フラグ (VIRTQ_DESC_F_*)
    uint16_t next;   // 次のディスクリプタのインデックス
} __packed;

// availableリング (2.7.6 The Virtqueue Available Ring)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-490006
struct virtq_avail {
    uint16_t flags;   // フラグ
    uint16_t index;   // ringのどこまで書きこまれたかを示すインデックス
    uint16_t ring[];  // 各ディスクリプタチェーンの先頭インデックス
} __packed;

// usedリングの各エントリ (2.7.8 The Virtqueue Used Ring)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-540008
struct virtq_used_elem {
    uint32_t id;   // ディスクリプタチェーンの先頭のディスクリプタのインデックス
    uint32_t len;  // ディスクリプタチェーンの合計サイズ
} __packed;

// usedリング (2.7.8 The Virtqueue Used Ring)
// https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html#x1-540008
struct virtq_used {
    uint16_t flags;                 // フラグ
    uint16_t index;                 // ringのどこまで書きこまれたかを示すインデックス
    struct virtq_used_elem ring[];  // 各処理済みエントリ
} __packed;

// ディスクリプタチェーン。このvirtioライブラリで使われる構造体で、デバイスドライバはこの構造体を
// 通してデバイスへの処理要求・処理結果を扱う。
struct virtio_chain_entry {
    uint16_t desc_index;   // ディスクリプタテーブル上のインデックス (戻り値)
    paddr_t addr;          // DMA領域の (物理) メモリアドレス
    uint32_t len;          // DMA領域のサイズ
    bool device_writable;  // デバイスから見て書き込み可能か
};
