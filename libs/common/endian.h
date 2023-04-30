#pragma once
#include <libs/common/types.h>

// ビッグエンディアンとリトルエンディアンの相互変換 (16ビット)
static inline uint16_t swap16(uint16_t x) {
    return ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8);
}

// ビッグエンディアンとリトルエンディアンの相互変換 (32ビット)
static inline uint32_t swap32(uint32_t x) {
    return ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8)
           | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24);
}

// CPUのエンディアンを調べる
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
static inline uint16_t ntoh16(uint16_t x) {
    return x;  // ネットワークバイトオーダーはビッグエンディアンなのでそのまま
}
static inline uint32_t ntoh32(uint32_t x) {
    return x;  // ネットワークバイトオーダーはビッグエンディアンなのでそのまま
}
static inline uint16_t hton16(uint16_t x) {
    return x;  // ネットワークバイトオーダーはビッグエンディアンなのでそのまま
}
static inline uint32_t hton32(uint32_t x) {
    return x;  // ネットワークバイトオーダーはビッグエンディアンなのでそのまま
}
static inline uint16_t into_le16(uint16_t x) {
    return swap16(x);  // ビックエンディアン -> リトルエンディアン
}
static inline uint32_t into_le32(uint32_t x) {
    return swap32(x);  // ビックエンディアン -> リトルエンディアン
}
static inline uint16_t from_le16(uint16_t x) {
    return swap16(x);  // リトルエンディアン -> ビックエンディアン
}
static inline uint32_t from_le32(uint32_t x) {
    return swap32(x);  // リトルエンディアン -> ビックエンディアン
}
#else
static inline uint16_t ntoh16(uint16_t x) {
    return swap16(x);  // ネットワークバイトオーダー -> リトルエンディアン
}
static inline uint32_t ntoh32(uint32_t x) {
    return swap32(x);  // ネットワークバイトオーダー -> リトルエンディアン
}
static inline uint16_t hton16(uint16_t x) {
    return swap16(x);  // リトルエンディアン -> ネットワークバイトオーダー
}
static inline uint32_t hton32(uint32_t x) {
    return swap32(x);  // リトルエンディアン -> ネットワークバイトオーダー
}
static inline uint16_t into_le16(uint16_t x) {
    return x;  // リトルエンディアンなのでそのまま
}
static inline uint32_t into_le32(uint32_t x) {
    return x;  // リトルエンディアンなのでそのまま
}
static inline uint64_t into_le64(uint64_t x) {
    return x;  // リトルエンディアンなのでそのまま
}
static inline uint16_t from_le16(uint16_t x) {
    return x;  // リトルエンディアンなのでそのまま
}
static inline uint32_t from_le32(uint32_t x) {
    return x;  // リトルエンディアンなのでそのまま
}
#endif
