#pragma once
#include "mbuf.h"
#include <libs/common/endian.h>
#include <libs/common/print.h>
#include <libs/common/types.h>

// チェックサム計算の途中結果を表す型。チェックサムであることを明示するために新しい型を定義する。
typedef uint32_t checksum_t;

// チェックサム計算の初期化
static inline void checksum_init(checksum_t *c) {
    *c = 0;
}

// チェックサムに指定したバイト列を追加する
static inline void checksum_update(checksum_t *c, const void *data,
                                   size_t len) {
    // 各16ビットワードを加算する
    const uint16_t *words = data;
    for (size_t i = 0; i < len / 2; i++) {
        *c += words[i];
    }

    // 長さが奇数の場合は最後の1バイトを加算する
    if (len % 2 != 0) {
        *c += ((uint8_t *) data)[len - 1];
    }
}

// チェックサムに指定した2バイト整数を追加する
static inline void checksum_update_uint16(checksum_t *c, uint16_t data) {
    *c += data;
}

// チェックサムに指定した4バイト整数を追加する
static inline void checksum_update_uint32(checksum_t *c, uint32_t data) {
    *c += data & 0xffff;
    *c += data >> 16;
}

// チェックサムに指定したmbufの全データを追加する
static inline void checksum_update_mbuf(checksum_t *c, mbuf_t mbuf) {
    while (mbuf) {
        checksum_update(c, mbuf_data(mbuf), mbuf_len_one(mbuf));
        mbuf = mbuf->next;
    }
}

// チェックサムを集計して最終結果を返す
static inline uint32_t checksum_finish(checksum_t *c) {
    *c = (*c >> 16) + (*c & 0xffff);
    *c += *c >> 16;
    return ~*c;
}
