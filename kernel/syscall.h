#pragma once
#include <libs/common/types.h>

error_t memcpy_from_user(void *dst, __user const void *src, size_t len);
error_t memcpy_to_user(__user void *dst, const void *src, size_t len);
long handle_syscall(long a0, long a1, long a2, long a3, long a4, long n);
