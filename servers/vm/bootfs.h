#pragma once
#include <libs/common/types.h>

// BootFSファイルシステムヘッダ
struct bootfs_header {
    uint16_t version;
    uint16_t header_size;
    uint16_t num_files;
    uint16_t padding;
} __packed;

// BootFSファイルエントリ
struct bootfs_file {
    char name[56];
    uint32_t offset;
    uint32_t len;
} __packed;

struct bootfs_file *bootfs_open(const char *path);
struct bootfs_file *bootfs_open_iter(unsigned index);
void bootfs_read(struct bootfs_file *file, offset_t off, void *buf, size_t len);
void bootfs_init(void);
