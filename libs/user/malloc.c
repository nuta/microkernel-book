#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/malloc.h>

extern char __heap[];      // ヒープ領域の先頭アドレス
extern char __heap_end[];  // ヒープ領域の終端アドレス

// 未使用チャンクリスト
static list_t free_chunks = LIST_INIT(free_chunks);

// ptrからlenバイトのメモリ領域を新しいチャンクとして登録する。
static void insert(void *ptr, size_t len) {
    ASSERT(len > sizeof(struct malloc_chunk));

    // チャンクの各フィールドを初期化
    struct malloc_chunk *new_chunk = ptr;
    new_chunk->magic = MALLOC_FREE;
    new_chunk->capacity = len - sizeof(struct malloc_chunk);
    new_chunk->size = 0;
    list_elem_init(&new_chunk->next);

    // フリーリストに追加
    list_push_back(&free_chunks, &new_chunk->next);
}

// チャンクの容量をcapバイトへ縮小する。分割後の余り部分は、新しいチャンクとして
// フリーリストに追加される。
static void split(struct malloc_chunk *chunk, size_t cap) {
    ASSERT(chunk->capacity >= cap + sizeof(struct malloc_chunk) + 8);

    // 新しいチャンクの全体サイズを計算
    size_t new_chunk_size = chunk->capacity - cap;

    // 新しいチャンクをchunkのデータ領域の末尾に作り、その分chunkを縮小する。
    void *new_chunk = &chunk->data[chunk->capacity - new_chunk_size];
    chunk->capacity = cap;

    // 新しく作ったチャンクをフリーリストに登録する。
    insert(new_chunk, new_chunk_size);
}

// 動的メモリ割り当て。ヒープからメモリを割り当てる。C標準ライブラリと違い、メモリ割り当てに
// 失敗したときはプログラムを終了する。
void *malloc(size_t size) {
    // 要求サイズを8以上の8にアライメントされた数にする。
    // つまり、8、16、24、32、...という単位で割り当てる。
    size = ALIGN_UP((size == 0) ? 1 : size, 8);

    LIST_FOR_EACH (chunk, &free_chunks, struct malloc_chunk, next) {
        ASSERT(chunk->magic == MALLOC_FREE);

        if (chunk->capacity >= size) {
            // 分割可能なほど大きければ、次の2つのチャンクに分割する。
            // 余りチャンクはフリーリストに追加される。
            //
            // - 割り当てられるチャンク (割り当てサイズぴったり)
            // - 余りチャンク (最小割り当てサイズの8バイト以上)
            if (chunk->capacity >= size + sizeof(struct malloc_chunk) + 8) {
                split(chunk, size);
            }

            // 使用中であるマークをつけ、未使用チャンクリストから取り除いてから
            // アプリケーションにデータ部の先頭アドレスを返す。
            chunk->magic = MALLOC_IN_USE;
            chunk->size = size;
            list_remove(&chunk->next);

            // 割り当てられたメモリ領域をゼロクリアする。本来は呼び出し元が
            // きちんと初期化すべきだが、初期化し忘れバグのデバッグは大変なので
            // ここでやってしまう。
            memset(chunk->data, 0, chunk->size);
            return chunk->data;
        }
    }

    PANIC("out of memory");
}

// ポインタからチャンクヘッダを取得する。malloc関数で割り当てたポインタでない場合はパニックする。
static struct malloc_chunk *get_chunk_from_ptr(void *ptr) {
    struct malloc_chunk *chunk =
        (struct malloc_chunk *) ((uintptr_t) ptr - sizeof(struct malloc_chunk));

    ASSERT(chunk->magic == MALLOC_IN_USE);
    return chunk;
}

// malloc関数で割り当てたメモリ領域を解放する。
void free(void *ptr) {
    struct malloc_chunk *chunk = get_chunk_from_ptr(ptr);
    if (chunk->magic == MALLOC_FREE) {
        // 既に解放済みのメモリ領域を解放しようとした (double-freeバグ)
        PANIC("double-free bug!");
    }

    // チャンクをフリーリストに戻す
    chunk->magic = MALLOC_FREE;
    list_push_back(&free_chunks, &chunk->next);
}

// メモリ再割り当て。malloc関数で割り当てたメモリ領域をsizeバイトに拡張した
// 新しいメモリ領域を返す。
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }

    struct malloc_chunk *chunk = get_chunk_from_ptr(ptr);
    size_t prev_size = chunk->size;
    if (size <= chunk->capacity) {
        // 今のチャンクに十分あまりがある場合は、そのまま返す。
        return ptr;
    }

    // 新しいメモリ領域を割り当てて、データをコピーする。
    void *new_ptr = malloc(size);
    memcpy(new_ptr, ptr, prev_size);
    free(ptr);
    return new_ptr;
}

// 文字列を新しく割り当てたメモリ領域にコピーし、その先頭アドレスを返す。
// メモリリークを防ぐために、free関数で解放する必要がある。
char *strdup(const char *s) {
    size_t len = strlen(s);
    char *new_s = malloc(len + 1);
    memcpy(new_s, s, len + 1);
    return new_s;
}

// 動的メモリ割り当ての初期化。ヒープ領域を未使用チャンクリストに追加する。
void malloc_init(void) {
    // ヒープ領域 (__heap, __heap_end) はリンカースクリプトで定義される。
    insert(__heap, (size_t) __heap_end - (size_t) __heap);
}
