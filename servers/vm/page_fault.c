#include "page_fault.h"
#include "bootfs.h"
#include "task.h"
#include <libs/common/print.h>
#include <libs/user/syscall.h>

// ページフォルト処理。ページを用意してマップする。できなかった場合はエラーを返す。
error_t handle_page_fault(struct task *task, uaddr_t uaddr, uaddr_t ip,
                          unsigned fault) {
    if (uaddr < PAGE_SIZE) {
        // 0番地付近のアドレスはマップできないため、その付近へのアクセスはヌルポインタ参照
        // とみなす。なぜ uaddr == 0 ではないかというと、構造体へのヌルポインタに対して
        // メンバへのアクセスを試みる場合、uaddrはゼロではなくメンバのオフセットになるため。
        WARN("%s (%d): null pointer dereference at vaddr=%p, ip=%p", task->name,
             task->tid, uaddr, ip);
        return ERR_NOT_ALLOWED;
    }

    // ページ境界にアラインする。
    uaddr_t uaddr_original = uaddr;
    uaddr = ALIGN_DOWN(uaddr, PAGE_SIZE);

    if (fault & PAGE_FAULT_PRESENT) {
        // 既にページが存在する。アクセス権限が不正な場合、たとえば読み込み専用ページに
        // 書き込もうとした場合。
        WARN(
            "%s: invalid memory access at %p (IP=%p, reason=%s%s%s, perhaps segfault?)",
            task->name, uaddr_original, ip,
            (fault & PAGE_FAULT_READ) ? "read" : "",
            (fault & PAGE_FAULT_WRITE) ? "write" : "",
            (fault & PAGE_FAULT_EXEC) ? "exec" : "");
        return ERR_NOT_ALLOWED;
    }

    // ページフォルトが起きたアドレスを踏むセグメントを探す。
    elf_phdr_t *phdr = NULL;
    for (unsigned i = 0; i < task->ehdr->e_phnum; i++) {
        if (task->phdrs[i].p_type != PT_LOAD) {
            // PT_LOAD以外の、メモリ上に展開されないセグメントは無視する。
            continue;
        }

        // ページフォルトが起きたアドレスがセグメントの範囲内にあるかどうかを調べる。
        uaddr_t start = task->phdrs[i].p_vaddr;
        uaddr_t end = start + task->phdrs[i].p_memsz;
        if (start <= uaddr && uaddr < end) {
            phdr = &task->phdrs[i];
            break;
        }
    }

    // 該当するセグメントがない場合は無効なアドレスとみなす。
    if (!phdr) {
        ERROR("unknown memory address (addr=%p, IP=%p), killing %s...",
              uaddr_original, ip, task->name);
        return ERR_INVALID_ARG;
    }

    // 物理ページを用意する。
    pfn_t pfn_or_err = sys_pm_alloc(task->tid, PAGE_SIZE, 0);
    if (IS_ERROR(pfn_or_err)) {
        return pfn_or_err;
    }

    // pm_allocは物理ページ番号を返すので、物理アドレスに変換する。
    paddr_t paddr = PFN2PADDR(pfn_or_err);

    // 割り当てた物理ページにセグメントの内容をELFイメージからコピーする。
    size_t offset = uaddr - phdr->p_vaddr;
    if (offset < phdr->p_filesz) {
        // セグメントの内容をコピーするための仮想アドレス領域。ここで確保したメモリ領域が
        // 実際に使われず、この領域の仮想アドレスが他の物理ページにマップされる。
        static __aligned(PAGE_SIZE) uint8_t tmp_page[PAGE_SIZE];

        // tmp_pageを一旦アンマップする。カーネルによって起動時にマップされているため。
        ASSERT_OK(sys_vm_unmap(sys_task_self(), (uaddr_t) tmp_page));

        // tmp_pageをpaddrにマップする。これにより、tmp_pageの仮想アドレスを介して
        // paddrの内容にアクセスできるようになる。
        ASSERT_OK(sys_vm_map(sys_task_self(), (uaddr_t) tmp_page, paddr,
                             PAGE_READABLE | PAGE_WRITABLE));

        // BootFSからセグメントの内容を読み込む。
        size_t copy_len = MIN(PAGE_SIZE, phdr->p_filesz - offset);
        bootfs_read(task->file, phdr->p_offset + offset, tmp_page, copy_len);
    }

    // ページの属性をセグメント情報から決定する。
    unsigned attrs = 0;
    attrs |= (phdr->p_flags & PF_R) ? PAGE_READABLE : 0;
    attrs |= (phdr->p_flags & PF_W) ? PAGE_WRITABLE : 0;
    attrs |= (phdr->p_flags & PF_X) ? PAGE_EXECUTABLE : 0;

    // ページをマップする。
    ASSERT(phdr->p_filesz <= phdr->p_memsz);
    ASSERT_OK(sys_vm_map(task->tid, uaddr, paddr, attrs));
    return OK;
}
