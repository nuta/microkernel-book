#include "string.h"

// todo: replace with wasi-libc?
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

void *memcpy(void *dst, const void *src, size_t len) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (len-- > 0) {
        *d = *s;
        d++;
        s++;
    }
    return dst;
}