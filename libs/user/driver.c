// デバイスドライバAPI。基本的にはVMサーバに対するメッセージパッシングのラッパー。
#include <libs/user/driver.h>
#include <libs/user/ipc.h>

// 指定された物理メモリ領域を空いている仮想アドレス領域にマップする。MMIO領域にアクセスしたい
// 時に使える。
//
// 引数 map_flags にはメモリ領域の権限 PAGE_(READABLE|WRITABLE|EXECUTABLE) を指定する。
error_t driver_map_pages(paddr_t paddr, size_t size, int map_flags,
                         uaddr_t *uaddr) {
    struct message m;
    m.type = VM_MAP_PHYSICAL_MSG;
    m.vm_map_physical.paddr = paddr;
    m.vm_map_physical.size = size;
    m.vm_map_physical.map_flags = map_flags;
    error_t err = ipc_call(VM_SERVER, &m);
    if (err != OK) {
        return err;
    }

    *uaddr = m.vm_map_physical_reply.uaddr;
    return OK;
}

// 物理メモリ領域を確保する。
//
// 引数 map_flags にはメモリ領域の権限 PAGE_(READABLE|WRITABLE|EXECUTABLE) を指定する。
error_t driver_alloc_pages(size_t size, int map_flags, uaddr_t *uaddr,
                           paddr_t *paddr) {
    struct message m;
    m.type = VM_ALLOC_PHYSICAL_MSG;
    m.vm_alloc_physical.size = size;
    m.vm_alloc_physical.alloc_flags = 0;
    m.vm_alloc_physical.map_flags = map_flags;
    error_t err = ipc_call(VM_SERVER, &m);
    if (err != OK) {
        return err;
    }

    *uaddr = m.vm_alloc_physical_reply.uaddr;
    *paddr = m.vm_alloc_physical_reply.paddr;
    return OK;
}
