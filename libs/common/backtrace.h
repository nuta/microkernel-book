#pragma once
#include <libs/common/types.h>

#define BACKTRACE_MAX      16          // バックトレースの深さの最大値
#define SYMBOL_TABLE_MAGIC 0x4c4d5953  // "SYML"

// シンボルテーブルの各エントリ。
struct symbol {
    uint32_t addr;  // シンボルのアドレス
    char name[60];  // シンボル名
} __packed;

// シンボルテーブル。
struct symbol_table {
    uint32_t magic;           // SYMBOL_TABLE_MAGIC
    uint32_t num_symbols;     // シンボルの数
    uint64_t padding;         // パディング
    struct symbol symbols[];  // シンボルの配列。アドレス順にソートされている。
} __packed;

// ELFファイルに埋め込まれたシンボルテーブル。
extern struct symbol_table __symbol_table;

struct symbol *find_symbol(vaddr_t addr);
void backtrace(void);
