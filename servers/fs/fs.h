#pragma once

#include "block.h"
#include <libs/common/types.h>

#define FS_MAGIC           0xf2005346  // マジックナンバー
#define FS_HEADER_BLOCK    0           // ファイルシステムヘッダーのブロック番号
#define ROOT_DIR_BLOCK     1           // ルートディレクトリのブロック番号
#define BITMAP_FIRST_BLOCK 2           // ビットマップテーブルの最初のブロック番号
#define NUM_BITMAP_BLOCKS  4           // ビットマップテーブルのブロック数

#define BLOCKS_PER_ENTRY 1908          // 1エントリに含まれる最大データブロック数
#define FS_TYPE_DIR      0xdd          // ディレクトリエントリの種類: ディレクトリ
#define FS_TYPE_FILE     0xff          // ファイルエントリの種類: ファイル
#define FS_NAME_LEN      256           // エントリ名の最大長

// ファイルシステム上の各エントリ
//
// たとえば /foo/bar/hello.txt というファイルがあるとすると、次のような3エントリが存在する:
//
// - エントリ名が「foo」のディレクトリエントリ
// - エントリ名が「bar」のディレクトリエントリ
// - エントリ名が「hello.txt」のファイルエントリ
struct hinafs_entry {
    uint8_t type;            // エントリの種類 (ファイルかディレクトリか)
    uint8_t padding[3];      // パディング
    char name[FS_NAME_LEN];  // エントリ名 (終端はヌル文字)
    union {
        // ファイルエントリの場合のみ有効なフィールド
        struct {
            uint32_t size;  // ファイルサイズ
        };

        // ディレクトリエントリの場合のみ有効なフィールド
        struct {
            uint16_t num_entries;  // ディレクトリ内のエントリ数
            uint16_t padding2;     // パディング
        };
    };
    int64_t created_at;                // 作成日時 (使われていない)
    int64_t modified_at;               // 最終更新日時 (使われていない)
    block_t blocks[BLOCKS_PER_ENTRY];  // データブロックのリスト:
                                       // - ファイル: ファイルデータ
                                       // - ディレクトリ: ディレクトリ内の各エントリ
} __packed;

// ファイルシステムのヘッダ
struct hinafs_header {
    uint32_t magic;            // マジックナンバー。FS_MAGICでなければならない。
    uint32_t num_data_blocks;  // データブロック数。
    uint8_t padding[4088];     // パディング。ブロックサイズに合わせるために必要。

    // このヘッダの後ろに、次のデータが続く:
    // struct hinafs_entry root_dir;                          // ルートディレクトリ
    // uint8_t bitmap_blocks[num_bitmap_blocks * BLOCK_SIZE]; // ビットマップ
    // uint8_t blocks[num_data_blocks * BLOCK_SIZE];          // データブロック
} __packed;

STATIC_ASSERT(sizeof(struct hinafs_header) == BLOCK_SIZE,
              "hinafs_header size must be equal to block size");
STATIC_ASSERT(sizeof(struct hinafs_entry) == BLOCK_SIZE,
              "hinafs_entry size must be equal to block size");

error_t fs_find(const char *path, struct block **entry_block);
error_t fs_create(const char *path, uint8_t type);
error_t fs_readwrite(struct block *entry_block, void *buf, size_t size,
                     size_t offset, bool write);
error_t fs_readdir(const char *path, int index, struct hinafs_entry **entry);
error_t fs_delete(const char *path);
void fs_init(void);
