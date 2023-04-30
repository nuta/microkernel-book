#include <libs/common/print.h>
#include <libs/common/string.h>

// メモリの内容を比較する。
int memcmp(const void *p1, const void *p2, size_t len) {
    uint8_t *s1 = (uint8_t *) p1;
    uint8_t *s2 = (uint8_t *) p2;
    while (*s1 == *s2 && len > 0) {
        s1++;
        s2++;
        len--;
    }

    return (len > 0) ? *s1 - *s2 : 0;
}

// メモリ領域の各バイトを指定した値で埋める。
void *memset(void *dst, int ch, size_t len) {
    uint8_t *d = dst;
    while (len-- > 0) {
        *d = ch;
        d++;
    }
    return dst;
}

// メモリ領域をコピーする。
void *memcpy(void *dst, const void *src, size_t len) {
    DEBUG_ASSERT(len < 256 * 1024 * 1024 /* 256MiB */
                 && "too long memcpy (perhaps integer overflow?)");

    uint8_t *d = dst;
    const uint8_t *s = src;
    while (len-- > 0) {
        *d = *s;
        d++;
        s++;
    }
    return dst;
}

// メモリ領域をコピーする。重なりがあっても正しく動作する。
void *memmove(void *dst, const void *src, size_t len) {
    DEBUG_ASSERT(len < 256 * 1024 * 1024 /* 256MiB */
                 && "too long memmove (perhaps integer overflow?)");

    if ((uintptr_t) dst <= (uintptr_t) src) {
        memcpy(dst, src, len);
    } else {
        uint8_t *d = dst + len;
        const uint8_t *s = src + len;
        while (len-- > 0) {
            *d = *s;
            --d;
            --s;
        }
    }
    return dst;
}

// 文字列の長さを返す。
size_t strlen(const char *s) {
    size_t len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

// 文字列を比較する。同じなら0を返す。
int strcmp(const char *s1, const char *s2) {
    while (true) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }

        if (*s1 == '\0') {
            return 0;
        }

        s1++;
        s2++;
    }

    return 0;
}

// 指定した文字数まで文字列を比較する。同じなら0を返す。
int strncmp(const char *s1, const char *s2, size_t len) {
    while (len > 0) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }

        if (*s1 == '\0') {
            // Both `*s1` and `*s2` equal to '\0'.
            break;
        }

        s1++;
        s2++;
        len--;
    }

    return 0;
}

// 文字列をコピーする。宛先のバッファサイズを超える場合は、バッファに収まるだけをコピーする。
char *strcpy_safe(char *dst, size_t dst_len, const char *src) {
    ASSERT(dst_len > 0);

    size_t i = 0;
    while (i < dst_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

// 指定した文字を文字列から探し、その位置を返す。
char *strchr(const char *str, int c) {
    char *s = (char *) str;
    while (*s != '\0') {
        if (*s == c) {
            return s;
        }

        s++;
    }

    return NULL;
}

// 指定した文字列を文字列から探し、その位置を返す。
char *strstr(const char *haystack, const char *needle) {
    char *s = (char *) haystack;
    size_t needle_len = strlen(needle);
    while (*s != '\0') {
        if (!strncmp(s, needle, needle_len)) {
            return s;
        }

        s++;
    }

    return NULL;
}

// 文字列を数値に変換する。10進数のみ対応。
int atoi(const char *s) {
    int x = 0;
    while ('0' <= *s && *s <= '9') {
        x = (x * 10) + (*s - '0');
        s++;
    }

    return x;
}
