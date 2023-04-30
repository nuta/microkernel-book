#include "block.h"
#include "fs.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>
#include <servers/virtio_blk/virtio_blk.h>  // SECTOR_SIZE

// ブロックデバイスドライバサーバのタスクID。
static task_t blk_server;
// キャッシュされたブロックのリスト。
static list_t cached_blocks = LIST_INIT(cached_blocks);
// 変更済みブロックのリスト。ディスクに書き戻す必要がある。
static list_t dirty_blocks = LIST_INIT(dirty_blocks);

// ブロック番号をセクタ番号に変換する。
static uint64_t block_to_sector(block_t index) {
    return (index * BLOCK_SIZE) / SECTOR_SIZE;
}

// ブロックが変更済みかどうかを返す。
static bool block_is_dirty(struct block *block) {
    return list_is_linked(&block->dirty_next);
}

// ブロックをディスクに書き込む。
static void block_write(struct block *block) {
    unsigned sector_base = block_to_sector(block->index);
    // 各セクタごとに書き込む
    for (int offset = 0; offset < BLOCK_SIZE; offset += SECTOR_SIZE) {
        struct message m;
        m.type = BLK_WRITE_MSG;
        m.blk_write.sector = sector_base + (offset / SECTOR_SIZE);
        m.blk_write.data_len = SECTOR_SIZE;
        memcpy(m.blk_write.data, block->data + offset, SECTOR_SIZE);
        error_t err = ipc_call(blk_server, &m);
        if (err != OK) {
            OOPS("failed to write block %d: %s", block->index, err2str(err));
        }
    }
}

// ブロックをブロックキャッシュに読み込む。
error_t block_read(block_t index, struct block **block) {
    if (index == 0xffff) {
        OOPS("invalid block index: %x", index);
        return ERR_INVALID_ARG;
    }

    // 既にキャッシュされていれば、それを返す。
    LIST_FOR_EACH (b, &cached_blocks, struct block, cache_next) {
        if (b->index == index) {
            *block = b;
            return OK;
        }
    }

    // ブロックキャッシュのメモリ領域を確保して、各セクタを読み込む。
    TRACE("block %d is not in cache, reading from disk", index);
    struct block *new_block = malloc(sizeof(struct block));
    for (int offset = 0; offset < BLOCK_SIZE; offset += SECTOR_SIZE) {
        // デバイスドライバサーバに対して、セクタ読み込み要求を送る。
        struct message m;
        m.type = BLK_READ_MSG;
        m.blk_read.sector = block_to_sector(index) + (offset / SECTOR_SIZE);
        m.blk_read.len = SECTOR_SIZE;
        error_t err = ipc_call(blk_server, &m);

        if (err != OK) {
            OOPS("failed to read block %d: %s", index, err2str(err));
            free(new_block);
            return err;
        }

        if (m.type != BLK_READ_REPLY_MSG) {
            OOPS("unexpected reply message type \"%s\" (expected=%s)",
                 msgtype2str(m.type), msgtype2str(BLK_READ_REPLY_MSG));
            free(new_block);
            return ERR_UNEXPECTED;
        }

        if (m.blk_read_reply.data_len != SECTOR_SIZE) {
            OOPS("invalid data length from the device: %d",
                 m.blk_read_reply.data_len);
            free(new_block);
            return ERR_UNEXPECTED;
        }

        // ブロックキャッシュに読み込んだディスクデータをコピーする。
        memcpy(&new_block->data[offset], m.blk_read_reply.data, SECTOR_SIZE);
    }

    // ブロックキャッシュをリストに追加し、そのポインタを返す。
    new_block->index = index;
    list_elem_init(&new_block->cache_next);
    list_elem_init(&new_block->dirty_next);
    list_push_back(&cached_blocks, &new_block->cache_next);
    *block = new_block;
    return OK;
}

// ブロックを変更済みにする。
void block_mark_as_dirty(struct block *block) {
    if (!block_is_dirty(block)) {
        list_push_back(&dirty_blocks, &block->dirty_next);
    }
}

// 変更済みブロックをすべてディスクに書き込む。
void block_flush_all(void) {
    LIST_FOR_EACH (b, &dirty_blocks, struct block, dirty_next) {
        block_write(b);
        list_remove(&b->dirty_next);
    }
}

// ブロックキャッシュレイヤの初期化。
void block_init(void) {
    // デバイスドライバサーバのタスクIDを取得する。
    blk_server = ipc_lookup("blk_device");
}
