#pragma once
#include <libs/common/list.h>
#include <libs/common/types.h>

// 物理ページ管理構造体
struct page {
    struct task *owner;  // 所有者 (NULLならカーネルの内部データ構造)
    unsigned ref_count;  // 参照カウンタ:
                         // - 0: 空き
                         // - 1: 割り当て済み (まだマップされていない)
                         // - 2: マップ済み (1つのタスクでのみ使用中)
                         // - 3以上: マップ済み (複数のタスクで使用中。つまり共有メモリ)
    list_elem_t next;    // 所有者タスクのtask->pagesのリスト要素
};

// メモリゾーンの種類
enum memory_zone_type {
    MEMORY_ZONE_FREE,  // 空き領域
    MEMORY_ZONE_MMIO,  // MMIO領域
};

// メモリゾーン管理構造体
struct memory_zone {
    enum memory_zone_type type;  // 種類
    list_elem_t next;            // 各メモリゾーンを繋げたリストの要素
    paddr_t base;                // 先頭物理アドレス
    size_t num_pages;            // 物理ページ数
    struct page pages[];         // 物理ページ管理構造体の配列
};

struct task;
paddr_t pm_alloc(size_t size, struct task *owner, unsigned flags);
void pm_own_page(paddr_t paddr, struct task *owner);
void pm_free(paddr_t paddr, size_t size);
void pm_free_by_list(list_t *pages);
error_t vm_map(struct task *task, uaddr_t uaddr, paddr_t paddr, unsigned attrs);
error_t vm_unmap(struct task *task, uaddr_t uaddr);
void handle_page_fault(uaddr_t uaddr, vaddr_t ip, unsigned fault);

struct bootinfo;
void memory_init(struct bootinfo *bootinfo);
