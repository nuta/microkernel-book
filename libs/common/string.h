#pragma once
#include <libs/common/types.h>

int memcmp(const void *p1, const void *p2, size_t len);
void *memset(void *dst, int ch, size_t len);
void *memcpy(void *dst, const void *src, size_t len);
void *memmove(void *dst, const void *src, size_t len);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);
char *strcpy_safe(char *dst, size_t dst_len, const char *src);
char *strchr(const char *str, int c);
char *strstr(const char *haystack, const char *needle);
int atoi(const char *s);
