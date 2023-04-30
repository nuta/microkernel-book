#include "vm.h"
#include "asm.h"
#include "mp.h"
#include "plic.h"
#include "uart.h"
#include <kernel/arch.h>
#include <kernel/memory.h>
#include <kernel/printk.h>
#include <libs/common/string.h>

// カーネルメモリ領域がマップされたページテーブル。起動時に生成され、各タスクの作成時にこの
// ページテーブルの内容がコピーされる。
static struct arch_vm kernel_vm;

// PAGE_* マクロで指定したページ属性をSv32のそれに変換する。
static pte_t page_attrs_to_pte_flags(unsigned attrs) {
    return ((attrs & PAGE_READABLE) ? PTE_R : 0)
           | ((attrs & PAGE_WRITABLE) ? PTE_W : 0)
           | ((attrs & PAGE_EXECUTABLE) ? PTE_X : 0)
           | ((attrs & PAGE_USER) ? PTE_U : 0);
}

// ページテーブルエントリを構築する。
static pte_t construct_pte(paddr_t paddr, pte_t flags) {
    DEBUG_ASSERT((paddr & ~PTE_PADDR_MASK) == 0);
    return ((paddr >> 12) << 10) | flags;
}

// ページテーブルからページテーブルエントリを取得する。引数baseはページテーブルの物理アドレス、
// vaddrは探索対象の仮想アドレス、allocがtrueの場合はページテーブルが設定されていない場合に
// 新たに割り当てる。
//
// 成功時に引数pteにページテーブルエントリのアドレスを返す。
static error_t walk(paddr_t base, vaddr_t vaddr, bool alloc, pte_t **pte) {
    ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));

    pte_t *l1table = (pte_t *) arch_paddr_to_vaddr(base);  // 1段目のテーブル
    int index = PTE_INDEX(1, vaddr);                       // 1段目のインデックス
    if (l1table[index] == 0) {
        // 2段目のテーブルが設定されていなかった
        if (!alloc) {
            return ERR_NOT_FOUND;
        }

        // 2段目のテーブルを割り当てる
        paddr_t paddr = pm_alloc(PAGE_SIZE, NULL, PM_ALLOC_ZEROED);
        if (!paddr) {
            return ERR_NO_MEMORY;
        }

        l1table[index] = construct_pte(paddr, PTE_V);  // 1段目のテーブルに登録
    }

    // 2段目のテーブル
    pte_t *l2table = (pte_t *) arch_paddr_to_vaddr(PTE_PADDR(l1table[index]));
    // vaddrのページテーブルエントリへのポインタ
    *pte = &l2table[PTE_INDEX(0, vaddr)];
    return OK;
}

// ページをマップする。
error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr,
                    unsigned attrs) {
    DEBUG_ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));

    // ページテーブルエントリを探す
    pte_t *pte;
    error_t err = walk(vm->table, vaddr, true, &pte);
    if (err != OK) {
        return err;
    }

    // 既にページがマップされていたら中断する
    DEBUG_ASSERT(pte != NULL);
    if (*pte & PTE_V) {
        return ERR_ALREADY_EXISTS;
    }

    // ページテーブルエントリを設定する
    *pte = construct_pte(paddr, page_attrs_to_pte_flags(attrs) | PTE_V);

    // TLBをクリアする
    asm_sfence_vma();

    // 他のCPUにTLBをクリアするよう通知する (TLB shootdown)
    arch_send_ipi(IPI_TLB_FLUSH);
    return OK;
}

// ページをアンマップする。
error_t arch_vm_unmap(struct arch_vm *vm, vaddr_t vaddr) {
    // ページテーブルエントリを探す
    pte_t *pte;
    error_t err = walk(vm->table, vaddr, false, &pte);
    if (err != OK) {
        return err;
    }

    // ページがマップされていなかったら中断する
    if (!pte || (*pte & PTE_V) == 0) {
        return ERR_NOT_FOUND;
    }

    // ページを解放する
    paddr_t paddr = PTE_PADDR(*pte);
    *pte = 0;
    pm_free(paddr, PAGE_SIZE);

    // TLBをクリアする
    asm_sfence_vma();

    // 他のCPUにTLBをクリアするよう通知する (TLB shootdown)
    arch_send_ipi(IPI_TLB_FLUSH);
    return OK;
}

// 仮想アドレスがページテーブルにマップされているかどうかを返す。
bool riscv32_is_mapped(uint32_t satp, vaddr_t vaddr) {
    satp = (satp & SATP_PPN_MASK) << SATP_PPN_SHIFT;
    uint32_t *pte;
    error_t err = walk(satp, ALIGN_DOWN(vaddr, PAGE_SIZE), false, &pte);
    return err == OK && pte != NULL && (*pte & PTE_V);
}

// ページテーブルを初期化する。
error_t arch_vm_init(struct arch_vm *vm) {
    // ページテーブル (1段目) を割り当てる
    vm->table = pm_alloc(PAGE_SIZE, NULL, PM_ALLOC_ZEROED);
    if (!vm->table) {
        return ERR_NO_MEMORY;
    }

    // カーネル空間のマッピングをコピーする
    memcpy((void *) arch_paddr_to_vaddr(vm->table),
           (void *) arch_paddr_to_vaddr(kernel_vm.table), PAGE_SIZE);
    return OK;
}

// ページテーブルを破棄する。
void arch_vm_destroy(struct arch_vm *vm) {
    // 仮想アドレスを走査して、ユーザ空間のページを解放する
    uint32_t *l1table = (uint32_t *) arch_paddr_to_vaddr(vm->table);
    for (int i = 0; i < 512; i++) {
        uint32_t pte1 = l1table[i];
        // エントリが設定されていなければスキップ
        if (!(pte1 & PTE_V)) {
            continue;
        }

        // 2段目のテーブル
        uint32_t *l2table = (uint32_t *) arch_paddr_to_vaddr(PTE_PADDR(pte1));
        for (int j = 0; j < 512; j++) {
            uint32_t pte2 = l2table[j];

            // ユーザ空間のページでなければスキップ
            if ((pte2 & (PTE_V | PTE_U)) != (PTE_V | PTE_U)) {
                continue;
            }

            // ページを解放する
            paddr_t paddr = PTE_PADDR(pte2);
            pm_free(paddr, PAGE_SIZE);
        }
    }

    // 1段目のページテーブルを格納している物理ページを解放する
    pm_free(vm->table, PAGE_SIZE);
}

// 連続領域をマップする。
static error_t map_pages(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr,
                         size_t size, unsigned attrs) {
    // 各ページをひとつずつマップする
    for (offset_t offset = 0; offset < size; offset += PAGE_SIZE) {
        error_t err = arch_vm_map(vm, vaddr + offset, paddr + offset, attrs);
        if (err != OK) {
            return err;
        }
    }

    return OK;
}

// ページング管理機構を初期化する。
void riscv32_vm_init(void) {
    // カーネルメモリのためのページテーブルを割り当てる
    kernel_vm.table = pm_alloc(PAGE_SIZE, NULL, PM_ALLOC_ZEROED);
    ASSERT(kernel_vm.table != 0);

    // リンカースクリプトで定義されている各アドレスを取得する
    vaddr_t kernel_text = (vaddr_t) __text;
    vaddr_t kernel_text_end = (vaddr_t) __text_end;
    vaddr_t kernel_data = (vaddr_t) __data;
    vaddr_t kernel_data_end = (vaddr_t) __data_end;
    vaddr_t ram_start = (vaddr_t) __ram_start;
    vaddr_t free_ram_start = (vaddr_t) __free_ram_start;

    DEBUG_ASSERT(IS_ALIGNED(kernel_text, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(kernel_text_end, PAGE_SIZE));

    // 自由に利用できる物理メモリのサイズ (カーネルメモリの静的領域を除いたもの)
    paddr_t free_ram_size = RAM_SIZE - (free_ram_start - ram_start);
    // カーネルのコード領域とデータ領域のサイズ
    size_t kernel_text_size = kernel_text_end - kernel_text;
    size_t kernel_data_size = kernel_data_end - kernel_data;

    // カーネルのコード領域
    ASSERT_OK(map_pages(&kernel_vm, kernel_text, kernel_text, kernel_text_size,
                        PAGE_WRITABLE | PAGE_READABLE | PAGE_EXECUTABLE));
    // カーネルのデータ領域
    ASSERT_OK(map_pages(&kernel_vm, kernel_data, kernel_data, kernel_data_size,
                        PAGE_READABLE | PAGE_WRITABLE));
    // カーネルが物理メモリ領域全体にアクセスできるようマップする
    ASSERT_OK(map_pages(&kernel_vm, free_ram_start, free_ram_start,
                        free_ram_size, PAGE_READABLE | PAGE_WRITABLE));
    // UART
    ASSERT_OK(map_pages(&kernel_vm, UART_ADDR, UART_ADDR, PAGE_SIZE,
                        PAGE_READABLE | PAGE_WRITABLE));
    // PLIC
    ASSERT_OK(map_pages(&kernel_vm, PLIC_ADDR, PLIC_ADDR, PLIC_SIZE,
                        PAGE_READABLE | PAGE_WRITABLE));
    // CLINT
    ASSERT_OK(map_pages(&kernel_vm, CLINT_PADDR, CLINT_PADDR, CLINT_SIZE,
                        PAGE_READABLE | PAGE_WRITABLE));
    // ACLINT
    ASSERT_OK(map_pages(&kernel_vm, ACLINT_SSWI_PADDR, ACLINT_SSWI_PADDR,
                        PAGE_SIZE, PAGE_READABLE | PAGE_WRITABLE));
}
