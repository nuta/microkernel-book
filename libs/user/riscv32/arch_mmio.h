#pragma once
#include <libs/common/endian.h>
#include <libs/common/types.h>

// 指定されたメモリアドレスに1バイトを書き込む。必ず1バイト幅のメモリアクセス命令を使う。
static inline void mmio_write8(vaddr_t vaddr, uint8_t val) {
    __asm__ __volatile__("sb %0, (%1)" ::"r"(val), "r"(vaddr) : "memory");
}

// 指定されたメモリアドレスに2バイトを書き込む。必ず2バイト幅のメモリアクセス命令を使う。
static inline void mmio_write16(vaddr_t vaddr, uint32_t val) {
    __asm__ __volatile__("sh %0, (%1)" ::"r"(val), "r"(vaddr) : "memory");
}

// 指定されたメモリアドレスに4バイトを書き込む。必ず4バイト幅のメモリアクセス命令を使う。
static inline void mmio_write32(vaddr_t vaddr, uint32_t val) {
    __asm__ __volatile__("sw %0, (%1)" ::"r"(val), "r"(vaddr) : "memory");
}

// 指定されたメモリアドレスから1バイトを読み込む。必ず1バイト幅のメモリアクセス命令を使う。
static inline uint8_t mmio_read8(vaddr_t vaddr) {
    uint8_t val;
    __asm__ __volatile__("lb %0, (%1)" : "=r"(val) : "r"(vaddr));
    return val;
}

// 指定されたメモリアドレスから2バイトを読み込む。必ず2バイト幅のメモリアクセス命令を使う。
static inline uint32_t mmio_read16(vaddr_t vaddr) {
    uint32_t val;
    __asm__ __volatile__("lh %0, (%1)" : "=r"(val) : "r"(vaddr));
    return val;
}

// 指定されたメモリアドレスから4バイトを読み込む。必ず4バイト幅のメモリアクセス命令を使う。
static inline uint32_t mmio_read32(vaddr_t vaddr) {
    uint32_t val;
    __asm__ __volatile__("lw %0, (%1)" : "=r"(val) : "r"(vaddr));
    return val;
}

// 指定されたメモリアドレスから16ビット整数 (リトルエンディアン) を読み込み、実行環境の
// エンディアンに変換して返す。
static inline uint16_t mmio_read16le(vaddr_t addr) {
    return from_le16(mmio_read16(addr));
}

// 指定されたメモリアドレスから32ビット整数 (リトルエンディアン) を読み込み、実行環境の
// エンディアンに変換して返す。
static inline uint32_t mmio_read32le(vaddr_t addr) {
    return from_le32(mmio_read32(addr));
}

// 指定されたメモリアドレスに16ビット整数 (リトルエンディアン) を書き込む。実行環境の
// エンディアンをリトルエンディアンに変換してから書き込む。
static inline void mmio_write16le(vaddr_t addr, uint16_t val) {
    mmio_write16(addr, into_le16(val));
}

// 指定されたメモリアドレスに32ビット整数 (リトルエンディアン) を書き込む。実行環境の
// エンディアンをリトルエンディアンに変換してから書き込む。
static inline void mmio_write32le(vaddr_t addr, uint32_t val) {
    mmio_write32(addr, into_le32(val));
}
