#pragma once
#include <libs/common/types.h>

error_t driver_map_pages(paddr_t paddr, size_t size, int map_flags,
                         uaddr_t *uaddr);
error_t driver_alloc_pages(size_t size, int map_flags, uaddr_t *uaddr,
                           paddr_t *paddr);
