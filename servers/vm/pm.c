#include "pm.h"
#include "task.h"
#include <libs/common/print.h>
#include <libs/user/syscall.h>

// タスクで使われていない仮想アドレス領域を返す。仮想アドレスは割り当てっぱなしで解放はできない。
static uaddr_t valloc(struct task *task, size_t size) {
    if (task->valloc_next >= VALLOC_END) {
        return 0;
    }

    uaddr_t uaddr = task->valloc_next;
    task->valloc_next += ALIGN_UP(size, PAGE_SIZE);
    return uaddr;
}

// 物理アドレスをタスクのページテーブルにマップする。uaddrには割り当てた仮想アドレスが返る。
error_t map_pages(struct task *task, size_t size, int map_flags, paddr_t paddr,
                  uaddr_t *uaddr) {
    DEBUG_ASSERT(IS_ALIGNED(size, PAGE_SIZE));
    *uaddr = valloc(task, size);
    if (!*uaddr) {
        return ERR_NO_RESOURCES;
    }

    // 各ページをマップする。
    for (offset_t offset = 0; offset < size; offset += PAGE_SIZE) {
        error_t err =
            sys_vm_map(task->tid, *uaddr + offset, paddr + offset, map_flags);
        if (err != OK) {
            WARN("vm_map failed: %s", err2str(err));
            return err;
        }
    }

    return OK;
}

// 物理ページを割り当てて、タスクのページテーブルにマップする。uaddrには割り当てた仮想アドレスが返る。
error_t alloc_pages(struct task *task, size_t size, int alloc_flags,
                    int map_flags, paddr_t *paddr, uaddr_t *uaddr) {
    pfn_t pfn = sys_pm_alloc(task->tid, size,
                             alloc_flags | PM_ALLOC_ALIGNED | PM_ALLOC_ZEROED);
    if (IS_ERROR(pfn)) {
        return pfn;
    }

    *paddr = PFN2PADDR(pfn);
    return map_pages(task, size, map_flags, *paddr, uaddr);
}
