#pragma once
#include <libs/common/types.h>

// mbufのデータ部分のサイズ
#define MBUF_MAX_LEN (512 - (sizeof(struct mbuf *) + 2 * sizeof(uint16_t)))

// mbuf: 単方向リストで構成される非連続メモリバッファ
struct mbuf {
    struct mbuf *next;           // 次のmbufへのポインタ
    uint16_t offset;             // 有効なデータの先頭オフセット
    uint16_t offset_end;         // 有効化データの終端オフセット
    uint8_t data[MBUF_MAX_LEN];  // データ
};

// mbufを表す型。いわゆるopaqueポインタ。
typedef struct mbuf *mbuf_t;

mbuf_t mbuf_alloc(void);
void mbuf_delete(mbuf_t mbuf);
mbuf_t mbuf_new(const void *data, size_t len);
void mbuf_append(mbuf_t mbuf, mbuf_t new_tail);
void mbuf_append_bytes(mbuf_t mbuf, const void *data, size_t len);
const void *mbuf_data(mbuf_t mbuf);
size_t mbuf_len_one(mbuf_t mbuf);
size_t mbuf_len(mbuf_t mbuf);
bool mbuf_is_empty(mbuf_t mbuf);
size_t mbuf_read(mbuf_t *mbuf, void *buf, size_t buf_len);
mbuf_t mbuf_peek(mbuf_t mbuf, size_t len);
size_t mbuf_discard(mbuf_t *mbuf, size_t len);
void mbuf_truncate(mbuf_t mbuf, size_t len);
mbuf_t mbuf_clone(mbuf_t mbuf);
