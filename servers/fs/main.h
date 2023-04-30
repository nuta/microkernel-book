#pragma once
#include <libs/common/types.h>

#define WRITE_BACK_INTERVAL 1000
#define OPEN_FILES_MAX      64

// 開いているファイルの情報
struct open_file {
    bool used;                   // この管理構造体を利用中か
    task_t task;                 // このファイルを開いているタスク
    struct hinafs_entry *entry;  // ファイルのエントリ
    struct block *entry_block;   // ファイルのエントリがあるブロック
    uint32_t offset;             // 現在のオフセット (読み書き操作をすると動く)
};
