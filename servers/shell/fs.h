#pragma once
#include <libs/common/types.h>

void fs_read(const char *path);
void fs_write(const char *path, const uint8_t *buf, size_t len);
void fs_listdir(const char *path);
void fs_mkdir(const char *path);
void fs_delete(const char *path);
