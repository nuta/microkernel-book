#pragma once
#include <libs/common/types.h>

#define ELF_MAGIC "\177ELF"  // ELFファイルのマジックナンバー
#define ET_EXEC   2          // 実行可能ファイル
#define PT_LOAD   1          // メモリ上に展開されるセグメント

#define PF_R (1 << 2)        // 読み込み可能領域
#define PF_W (1 << 1)        // 書き込み可能領域
#define PF_X (1 << 0)        // 実行可能領域

// ELFヘッダ (32ビット版)
struct elf32_ehdr {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __packed;

// プログラムヘッダ (32ビット版)
struct elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __packed;

#ifdef __LP64__
#    error "elf.h: __LP64__ not supported"
#else
typedef struct elf32_ehdr elf_ehdr_t;
typedef struct elf32_phdr elf_phdr_t;
#endif
