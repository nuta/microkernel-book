#pragma once
#include "task.h"

error_t alloc_pages(struct task *task, size_t size, int alloc_flags,
                    int map_flags, paddr_t *paddr, uaddr_t *uaddr);
error_t map_pages(struct task *task, size_t size, int map_flags, paddr_t paddr,
                  uaddr_t *uaddr);
