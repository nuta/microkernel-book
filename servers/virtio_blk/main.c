#include "virtio_blk.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/dmabuf.h>
#include <libs/user/driver.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>
#include <libs/user/virtio/virtio_mmio.h>

static struct virtio_mmio device;      // virtioデバイスの管理構造体
static struct virtio_virtq *requestq;  // 読み書き処理要求用virtqueue
static dmabuf_t dmabuf;                // 読み書き処理要求用virtqueueで使われるバッファ

// ディスクの読み書き
static error_t read_write(task_t task, uint64_t sector, void *buf, size_t len,
                          bool is_write) {
    // 読み込むバイト数はセクタサイズにアラインされている必要がある
    if (!IS_ALIGNED(len, SECTOR_SIZE)) {
        return ERR_INVALID_ARG;
    }

    // 大きすぎてもダメ
    if (len > REQUEST_BUFFER_SIZE) {
        return ERR_TOO_LARGE;
    }

    // 処理要求用のバッファを割り当てる
    struct virtio_blk_req *req;
    paddr_t paddr;
    if ((req = dmabuf_alloc(dmabuf, &paddr)) == NULL) {
        WARN("no free TX buffers");
        return ERR_TRY_AGAIN;
    }

    // 処理要求を埋める
    req->type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    req->reserved = 0;
    req->sector = sector;
    if (is_write) {
        memcpy(req->data, buf, len);
    }

    // ディスクリプタチェーン[0]: type, reserved, sector (デバイスからは読み込み専用)
    struct virtio_chain_entry chain[3];
    chain[0].addr = paddr;
    chain[0].len = sizeof(uint32_t) * 2 + sizeof(uint64_t);
    chain[0].device_writable = false;
    // ディスクリプタチェーン[1]: 書き込み元/読み込み先バッファ
    chain[1].addr = paddr + offsetof(struct virtio_blk_req, data);
    chain[1].len = len;
    chain[1].device_writable = !is_write;
    // ディスクリプタチェーン[2]: 処理結果用メモリ領域。デバイスが書き込む。
    chain[2].addr = paddr + offsetof(struct virtio_blk_req, status);
    chain[2].len = sizeof(uint8_t);
    chain[2].device_writable = true;

    // virtqueueにディスクリプタチェーンを追加
    int index_or_err = virtq_push(requestq, chain, 3);
    if (IS_ERROR(index_or_err)) {
        return index_or_err;
    }

    // virtio-blkに通知
    virtq_notify(&device, requestq);

    // 処理を終えるまでビジーウェイトする
    while (virtq_is_empty(requestq))
        ;

    // virtqueueから処理が終わったディスクリプタチェーンを取り出す
    size_t total_len;
    int n = virtq_pop(requestq, chain, 3, &total_len);
    if (IS_ERROR(n)) {
        WARN("virtq_pop returned an error: %s", err2str(n));
        dmabuf_free(dmabuf, paddr);
        return ERR_UNEXPECTED;
    }

    // ディスクリプタチェーンの内容を確認する。ここで送ったディスクリプタチェーンに対する
    // 応答が返ってきているはず。
    ASSERT(n == 3);
    ASSERT(chain[0].desc_index == index_or_err);
    ASSERT(chain[1].len == len);
    ASSERT(req->status == VIRTIO_BLK_S_OK);

    // 読み込み処理の場合は読み込んだデータをメモリバッファにコピーする
    if (!is_write) {
        memcpy(buf, req->data, len);
    }

    dmabuf_free(dmabuf, paddr);
    return OK;
}

// virtio-blkデバイスを初期化する
static void init_device(void) {
    // virtioデバイスを初期化する
    ASSERT_OK(virtio_init(&device, VIRTIO_BLK_PADDR, 1));

    // デバイスの機能を有効化する。特に必要な機能はないので、デバイスが提案した機能をそのまま
    // 有効化する。
    uint64_t features = virtio_read_device_features(&device);
    ASSERT_OK(virtio_negotiate_feature(&device, features));

    // デバイスを有効化する。
    ASSERT_OK(virtio_enable(&device));

    // virtqueueへのポインタを取得する。
    requestq = virtq_get(&device, 0);

    // 割り込みは使わないので無効化しておく。代わりにビジーウェイトで処理の完了を待つ。
    requestq->avail->flags |= VIRTQ_AVAIL_F_NO_INTERRUPT;

    // 処理要求用のDMAバッファを作成する。
    dmabuf = dmabuf_create(sizeof(struct virtio_blk_req), NUM_REQUEST_BUFFERS);
}

void main(void) {
    // virtio-blkデバイスを初期化する
    init_device();

    // ブロックデバイスとして登録する
    ASSERT_OK(ipc_register("blk_device"));
    TRACE("ready");

    while (true) {
        struct message m;
        ASSERT_OK(ipc_recv(IPC_ANY, &m));
        switch (m.type) {
            case BLK_READ_MSG: {
                uint8_t buf[SECTOR_SIZE];
                size_t len = m.blk_read.len;
                error_t err =
                    read_write(m.src, m.blk_read.sector, buf, len, false);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = BLK_READ_REPLY_MSG;
                m.blk_read_reply.data_len = m.blk_read.len;
                memcpy(m.blk_read_reply.data, buf, len);
                ipc_reply(m.src, &m);
                break;
            }
            case BLK_WRITE_MSG: {
                error_t err =
                    read_write(m.src, m.blk_write.sector, m.blk_write.data,
                               m.blk_write.data_len, true);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = BLK_WRITE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                WARN("unhandled message: %d", m.type);
                break;
        }
    }
}
