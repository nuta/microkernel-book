#include "main.h"
#include "arch.h"
#include "memory.h"
#include "printk.h"
#include "task.h"
#include <libs/common/elf.h>
#include <libs/common/string.h>

// 最初のユーザータスク (VMサーバ) を作成する
static void create_first_task(struct bootinfo *bootinfo) {
    // ELFヘッダのポインタを取得し、マジックナンバーを確認する
    elf_ehdr_t *header = (elf_ehdr_t *) arch_paddr_to_vaddr(bootinfo->boot_elf);
    if (memcmp(header->e_ident, ELF_MAGIC, 4) != 0) {
        PANIC("bootelf: invalid ELF magic\n");
    }

    // 最初のタスク (VMサーバ) を作成する
    task_t tid = task_create("vm", header->e_entry, NULL);
    ASSERT_OK(tid);
    struct task *task = task_find(tid);

    // プログラムヘッダを読み込む
    elf_phdr_t *phdrs = (elf_phdr_t *) ((vaddr_t) header + header->e_phoff);
    for (uint16_t i = 0; i < header->e_phnum; i++) {
        elf_phdr_t *phdr = &phdrs[i];

        // PT_LOAD 以外のセグメント (マップする必要のないセグメント) は無視する
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        ASSERT(phdr->p_memsz >= phdr->p_filesz);

        // デバッグ用にセグメントの情報を出力する
        char r = (phdr->p_flags & PF_R) ? 'r' : '-';
        char w = (phdr->p_flags & PF_W) ? 'w' : '-';
        char x = (phdr->p_flags & PF_X) ? 'x' : '-';
        size_t size_in_kb = phdr->p_memsz / 1024;
        TRACE("bootelf: %p - %p %c%c%c (%d KiB)", phdr->p_vaddr,
              phdr->p_vaddr + phdr->p_memsz, r, w, x, size_in_kb);

        // セグメントのマップ先となる物理メモリ領域を確保する
        paddr_t paddr = pm_alloc(phdr->p_memsz, task, PM_ALLOC_ZEROED);
        ASSERT(paddr != 0);

        // セグメントの内容を確保したメモリ領域にコピーする
        memcpy((void *) arch_paddr_to_vaddr(paddr),
               (void *) ((vaddr_t) header + phdr->p_offset), phdr->p_filesz);

        // セグメントが必要とするページのアクセス権限を取り出す
        unsigned attrs = PAGE_USER;
        attrs |= (phdr->p_flags & PF_R) ? PAGE_READABLE : 0;
        attrs |= (phdr->p_flags & PF_W) ? PAGE_WRITABLE : 0;
        attrs |= (phdr->p_flags & PF_X) ? PAGE_EXECUTABLE : 0;

        // タスクの各ページをマップする
        size_t memsz = ALIGN_UP(phdr->p_memsz, PAGE_SIZE);
        for (offset_t offset = 0; offset < memsz; offset += PAGE_SIZE) {
            error_t err =
                vm_map(task, phdr->p_vaddr + offset, paddr + offset, attrs);
            if (err != OK) {
                PANIC("bootelf: failed to map %p - %p", phdr->p_vaddr,
                      phdr->p_vaddr + phdr->p_memsz);
            }
        }
    }
}

// アイドルタスク: 他のタスクが実行可能な状態になるまで割り込み可能状態でCPUをスリープさせる。
__noreturn static void idle_task(void) {
    for (;;) {
        task_switch();
        arch_idle();
    }
}

// 0番目のCPUのブート処理: カーネルと最初のタスク (VMサーバ) を初期化した後にアイドルタスク
// として動作する。
void kernel_main(struct bootinfo *bootinfo) {
    printf("Booting WASMOS...\n");
    memory_init(bootinfo);
    arch_init();
    task_init_percpu();
    create_first_task(bootinfo);
    arch_init_percpu();
    TRACE("CPU #%d is ready", CPUVAR->id);

    // ここからアイドルタスクとして動作する
    idle_task();
}

// 0番目以外のCPUのブート処理: 各CPUの初期化処理を行った後にアイドルタスクとして動作する。
void kernel_mp_main(void) {
    task_init_percpu();
    arch_init_percpu();
    TRACE("CPU #%d is ready", CPUVAR->id);

    // ここからアイドルタスクとして動作する
    idle_task();
}
