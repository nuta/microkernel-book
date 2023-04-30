#pragma once
#include <libs/common/list.h>
#include <libs/common/types.h>

#define MALLOC_FREE   0x0a110ced  // チャンクが空き状態
#define MALLOC_IN_USE 0xdea110cd  // チャンクが使用中状態

// チャンク (mallocの割り当て単位) の管理構造体
struct malloc_chunk {
    list_elem_t next;    // 空きチャンクのリストの要素
    size_t capacity;     // チャンクのサイズ
    size_t size;         // ユーザが使っているサイズ (size <= capacity)
    uint32_t magic;      // チャンクの状態を表すマジックナンバー
    uint8_t padding[4];  // 8バイトアラインメントのためのパディング
    uint8_t data[];      // ユーザが使う可変長領域 (mallocが返すアドレス)
};

STATIC_ASSERT(IS_ALIGNED(sizeof(struct malloc_chunk), 8),
              "malloc_chunk size must be aligned to 8 bytes");

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
char *strdup(const char *s);
void free(void *ptr);
void malloc_init(void);
