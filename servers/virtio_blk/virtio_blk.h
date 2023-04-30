#pragma once
#include <libs/common/list.h>
#include <libs/common/message.h>
#include <libs/common/types.h>

#define VIRTIO_BLK_T_IN  0  // ディスクからの読み込み
#define VIRTIO_BLK_T_OUT 1  // ディスクへの書き込み

#define VIRTIO_BLK_S_OK 0   // 処理成功

// 読み書き処理要求用DMAバッファの数。逐次的に処理するため、ひとつで十分。
#define NUM_REQUEST_BUFFERS 1

// セクタのサイズ (バイト数)。ディスクの読み書きの最小単位。
#define SECTOR_SIZE 512

// 一度に読み書きできる最大バイト数。セクタサイズにアラインされている必要がある。
#define REQUEST_BUFFER_SIZE SECTOR_SIZE

STATIC_ASSERT(IS_ALIGNED(REQUEST_BUFFER_SIZE, SECTOR_SIZE),
              "virtio-blk buffer size must be aligned to the sector size");

// virtio-blkへの読み書き要求
struct virtio_blk_req {
    uint32_t type;                      // VIRTIO_BLK_T_IN または VIRTIO_BLK_T_OUT
    uint32_t reserved;                  // 予約済み
    uint64_t sector;                    // 読み書きするセクタ番号
    uint8_t data[REQUEST_BUFFER_SIZE];  // 読み書きするデータ
    uint8_t status;                     // 処理結果。成功ならばVIRTIO_BLK_S_OK。
} __packed;
