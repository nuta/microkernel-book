#include "fs.h"
#include "block.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/malloc.h>

// 空きブロックを管理するビットマップブロックに対応するブロックキャッシュ
static struct block *bitmap_blocks[NUM_BITMAP_BLOCKS];
// ルートディレクトリのブロックキャッシュ
static struct block *root_dir_block;

// 未使用のブロック番号を割り当て、ビットマップブロックに使用中であることを記録する。
static block_t alloc_block(void) {
    for (int i = 0; i < NUM_BITMAP_BLOCKS; i++) {
        struct block *b = bitmap_blocks[i];
        for (int j = 0; j < BLOCK_SIZE; j++) {
            // 各ビットを調べて、0のビットを見つけたら1にして返す。
            for (int k = 0; k < 8; k++) {
                if ((b->data[j] & (1 << k)) == 0) {
                    // ビットマップに使用中であることを記録する。
                    b->data[j] |= (1 << k);

                    // ビットマップブロックを変更済みにする。
                    block_mark_as_dirty(b);

                    block_t block_off = (i * BLOCK_SIZE + j) * 8 + k;
                    // ヘッダ・ルートディレクトリブロックがあるので、2を足す。
                    return 2 + NUM_BITMAP_BLOCKS + block_off;
                }
            }
        }
    }

    WARN("no free data blocks");
    return 0;
}

// ブロック番号を受け取り、ビットマップブロックに未使用であることを記録する。
static void free_block(block_t index) {
    int i = index / (BLOCK_SIZE * 8);
    int j = (index - (i * BLOCK_SIZE * 8)) / 8;
    int k = index % 8;

    struct block *b = bitmap_blocks[i];
    b->data[j] &= ~(1 << k);
    block_mark_as_dirty(b);
}

// ファイルパスからそのディレクトリエントリブロックを探す。parent_dirがtrueの場合は、
// パスが示すエントリの親ディレクトリを探す。
static error_t lookup(const char *path, bool parent_dir,
                      struct block **entry_block) {
    char *p = strdup(path);
    char *p_original = p;
    struct hinafs_entry *dir = (struct hinafs_entry *) root_dir_block->data;

    // 先頭のスラッシュを飛ばす。
    while (*p == '/') {
        p++;
    }

    // ルートディレクトリを指している時の処理。
    if (*p == '\0' || (parent_dir && !strchr(p, '/'))) {
        free(p_original);
        *entry_block = root_dir_block;
        return OK;
    }

    // パス中の各ディレクトリ (/path/to/file であれば path, to, file) を上から順に探す。
    while (true) {
        // 次のスラッシュまでの文字列を取り出し、ヌル文字で終端する。上記の例であれば、
        // p は "path", "to", "file" の文字列を指す。
        char *slash = strchr(p, '/');
        bool last = slash == NULL;
        if (!last) {
            *slash = '\0';
        }

        // 空文字列 (例えば /path//to/file など) や「.」はスキップする。
        if (strlen(p) == 0 || !strcmp(p, ".")) {
            p = slash + 1;
            continue;
        }

        // 「..」はエラーとする。
        if (!strcmp(p, "..")) {
            WARN(".. is not supported");
            return ERR_INVALID_ARG;
        }

        DEBUG_ASSERT(dir->type == FS_TYPE_DIR);

        // ディレクトリエントリの各エントリを順に調べて、名前が一致するものを探す。
        struct block *eb = NULL;
        bool found = false;
        for (uint16_t i = 0; i < dir->num_entries; i++) {
            // エントリブロックを読み込む。
            block_t index = dir->blocks[i];
            error_t err = block_read(index, &eb);
            if (err != OK) {
                WARN("failed to read block %d: %s", index, err2str(err));
                free(p_original);
                return err;
            }

            // 名前が一致するか調べる。
            struct hinafs_entry *e = (struct hinafs_entry *) eb->data;
            if (!strcmp(p, e->name)) {
                dir = e;
                found = true;
                break;
            }
        }

        // 一致するエントリが見つからなかった。存在しないパスなのでエラー。
        if (!found) {
            free(p_original);
            return ERR_NOT_FOUND;
        }

        // パスの最後までマッチしたら終了。
        if (last || (parent_dir && strchr(p + 1, '/') == NULL)) {
            free(p_original);
            *entry_block = eb;
            return OK;
        }

        // 次のディレクトリを探す。
        p = slash + 1;
    }

    UNREACHABLE();
}

// ファイルの読み書き
//
// ディレクトリエントリ (entry_block) が示すファイルに対して、読み込み・書き込みを行う。
static error_t readwrite(struct block *entry_block, void *buf, size_t size,
                         size_t offset, bool write) {
    // 本当にファイルなのかをチェックする
    struct hinafs_entry *entry = (struct hinafs_entry *) entry_block->data;
    if (entry->type != FS_TYPE_FILE) {
        return ERR_NOT_A_FILE;
    }

    // オフセットがファイルの長さを超えている場合はEOFを返す。ただし書き込みの場合は、
    // ファイルの末尾を拡張するケースを考慮する。
    bool valid_offset =
        offset < entry->size || (write && offset == entry->size);
    if (!valid_offset) {
        return ERR_EOF;
    }

    // 各データブロックに対して、読み込み・書き込みを行う。
    int first_i = offset / BLOCK_SIZE;
    int first_offset = offset % BLOCK_SIZE;
    size_t total_len = 0;  // 読み込んだ/書き込んだバイト数の合計
    for (int i = first_i; total_len < size && i < BLOCKS_PER_ENTRY; i++) {
        // データブロックが存在するかチェック
        block_t index = entry->blocks[i];
        if (!index) {
            DEBUG_ASSERT(write);

            // データブロックが存在しないので、新しく割り当てる。
            index = alloc_block();
            if (index == 0) {
                return ERR_NO_RESOURCES;
            }

            // 新しく割り当てたデータブロックをディレクトリエントリに追加する。
            entry->blocks[i] = index;
            // ディレクトリエントリを変更したので、変更済みブロックとして登録する。
            block_mark_as_dirty(entry_block);
        }

        // データブロックを読み込む。書き込み操作だとしても一旦読み込んでブロックキャッシュ上で
        // 変更する。
        struct block *data_block;
        error_t err = block_read(index, &data_block);
        if (err != OK) {
            WARN("failed to read block %d: %s", index, err2str(err));
            return err;
        }

        size_t copy_len = MIN(size - total_len, BLOCK_SIZE);
        if (write) {
            // データブロックへ書き込んで変更済みブロックとして登録する
            memcpy(&data_block->data[first_offset], buf + total_len, copy_len);
            block_mark_as_dirty(data_block);
        } else {
            // データブロックから読み込む
            memcpy(buf + total_len, &data_block->data[first_offset], copy_len);
        }

        total_len += copy_len;
        first_offset = 0;
    }

    if (write) {
        // ファイルの長さを更新し、ディレクトリエントリブロックを変更済みとして登録する
        entry->size = offset + total_len;
        block_mark_as_dirty(entry_block);
    }

    return OK;
}

// ファイルまたはディレクトリの削除。
error_t fs_delete(const char *path) {
    // 親ディレクトリを開く
    struct block *dir_block;
    error_t err = lookup(path, true, &dir_block);
    if (err != OK) {
        return err;
    }

    // 削除対象のディレクトリエントリを開く
    struct block *entry_block;
    err = lookup(path, false, &entry_block);
    if (err != OK) {
        return err;
    }

    struct hinafs_entry *entry = (struct hinafs_entry *) entry_block->data;
    switch (entry->type) {
        case FS_TYPE_FILE:
            // ファイルであれば、そのデータブロックをすべて開放する
            for (int i = 0; i < BLOCKS_PER_ENTRY; i++) {
                block_t index = entry->blocks[i];
                if (index == 0) {
                    break;
                }

                free_block(index);
            }
            break;
        case FS_TYPE_DIR:
            // ディレクトリであれば、既に空であるかどうかをチェックする
            if (entry->num_entries > 0) {
                return ERR_NOT_EMPTY;
            }
            break;
        default:
            UNREACHABLE();
    }

    // 親ディレクトリから削除対象のディレクトリエントリを削除する
    struct hinafs_entry *dir = (struct hinafs_entry *) dir_block->data;
    for (uint16_t i = 0; i < dir->num_entries; i++) {
        if (dir->blocks[i] == entry_block->index) {
            // 最後のエントリと削除対象のエントリを入れ替え、エントリ数を減らす。
            // 入れ替えることで、全てのエントリを1つずつずらす必要がない。
            dir->blocks[i] = dir->blocks[dir->num_entries - 1];
            dir->blocks[dir->num_entries - 1] = 0;
            dir->num_entries--;

            // エントリのリストを変更したので、親ディレクトリを変更済みとして登録する。
            block_mark_as_dirty(dir_block);
            break;
        }
    }

    return OK;
}

// ファイルパスの最後の要素を取り出す。例えば、"/foo/bar/baz" ならば "baz"
// を返す。
//
// 呼び出し元はpathが空文字列や "/" で終わらないことを保証する必要がある。
static const char *basename(const char *path) {
    const char *slash = &path[strlen(path)];
    while (true) {
        if (slash == path) {
            return path;
        }

        slash--;

        if (*slash == '/') {
            return slash + 1;
        }
    }
}

// ファイルまたはディレクトリの作成。
error_t fs_create(const char *path, uint8_t type) {
    const char *name = basename(path);
    // ファイル名が長すぎないかチェックする
    if (strlen(name) >= FS_NAME_LEN) {
        return ERR_INVALID_ARG;
    }

    // ファイル名が空文字列でないかチェックする
    if (strlen(name) == 0) {
        return ERR_INVALID_ARG;
    }

    // ファイル名に制御文字が含まれていないかチェックする
    for (size_t i = 0; i < strlen(name); i++) {
        if (name[i] < 0x20 || name[i] > 0x7e) {
            return ERR_INVALID_ARG;
        }
    }

    // ファイル名が重複していないかチェックする
    struct block *tmp_block;
    if (lookup(path, false, &tmp_block) == OK) {
        return ERR_ALREADY_EXISTS;
    }

    // 親ディレクトリを開く
    struct block *dir_block;
    error_t err = lookup(path, true, &dir_block);
    if (err != OK) {
        return err;
    }

    // ディレクトリエントリブロックを取得する
    struct hinafs_entry *dir = (struct hinafs_entry *) dir_block->data;
    if (dir->num_entries >= BLOCKS_PER_ENTRY) {
        // ディレクトリエントリが既に満杯
        return ERR_NO_RESOURCES;
    }

    // ディレクトリエントリ用の新しいブロックを割り当てる
    block_t new_index = alloc_block();
    if (new_index == 0) {
        return ERR_NO_RESOURCES;
    }

    // 割り当てたブロックをブロックキャッシュに載せる
    struct block *entry_block;
    err = block_read(new_index, &entry_block);
    if (err != OK) {
        WARN("failed to read block %d: %s", new_index, err2str(err));
        free_block(new_index);
        return err;
    }

    // ディレクトリエントリを初期化
    struct hinafs_entry *entry = (struct hinafs_entry *) entry_block->data;
    memset(entry, 0, sizeof(*entry));
    entry->type = type;
    entry->size = 0;
    strcpy_safe(entry->name, sizeof(entry->name), name);

    ASSERT(strchr(entry->name, '/') == NULL);

    // ディレクトリエントリをディレクトリに追加する
    dir->blocks[dir->num_entries] = new_index;
    dir->num_entries++;

    // ディスクに書き戻すようにマークする
    block_mark_as_dirty(dir_block);
    block_mark_as_dirty(entry_block);
    return OK;
}

// 指定されたパスに対応するディレクトリエントリを探す。
error_t fs_find(const char *path, struct block **entry_block) {
    return lookup(path, false, entry_block);
}

// ファイルの読み書き。
error_t fs_readwrite(struct block *entry_block, void *buf, size_t size,
                     size_t offset, bool write) {
    return readwrite(entry_block, buf, size, offset, write);
}

// ディレクトリのindex番目エントリをひとつ取得する。
error_t fs_readdir(const char *path, int index, struct hinafs_entry **entry) {
    // ディレクトリのブロックを読み込む
    struct block *dir_block;
    error_t err = lookup(path, false, &dir_block);
    if (err != OK) {
        return err;
    }

    // ディレクトリかどうかを確認する
    struct hinafs_entry *dir = (struct hinafs_entry *) dir_block->data;
    if (dir->type != FS_TYPE_DIR) {
        return ERR_NOT_A_DIR;
    }

    // indexがディレクトリのエントリ数を超えていないかを確認する
    if (index >= dir->num_entries) {
        return ERR_EOF;
    }

    // index番目のエントリを読み込む
    struct block *entry_block;
    block_t block_index = dir->blocks[index];
    err = block_read(block_index, &entry_block);
    if (err != OK) {
        WARN("failed to read block %d: %s", block_index, err2str(err));
        return err;
    }

    *entry = (struct hinafs_entry *) entry_block->data;
    return OK;
}

// ファイルシステムレイヤを初期化する。
void fs_init(void) {
    // ファイルシステムのヘッダを読み込む
    struct block *header_block;
    error_t err = block_read(FS_HEADER_BLOCK, &header_block);
    if (err != OK) {
        PANIC("failed to read the header block: %s", err2str(err));
    }

    // ファイルシステムのヘッダをチェックする
    struct hinafs_header *header = (struct hinafs_header *) header_block->data;
    if (header->magic != FS_MAGIC) {
        PANIC("invalid file system magic: %x", header->magic);
    }

    // ルートディレクトリを読み込む
    err = block_read(ROOT_DIR_BLOCK, &root_dir_block);
    if (err != OK) {
        PANIC("failed to read the root directory block: %s", err2str(err));
    }

    // ルートディレクトリをチェックする
    struct hinafs_entry *root_dir =
        (struct hinafs_entry *) root_dir_block->data;
    if (root_dir->type != FS_TYPE_DIR) {
        PANIC("invalid root directory type: %x", root_dir->type);
    }

    // 各ビットマップブロックを読み込む
    for (int i = 0; i < NUM_BITMAP_BLOCKS; i++) {
        err = block_read(BITMAP_FIRST_BLOCK + i, &bitmap_blocks[i]);
        if (err != OK) {
            PANIC("failed to read the bitmap block: %s", err2str(err));
        }
    }

    INFO("successfully loaded the file system");
}
