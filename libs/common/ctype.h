#pragma once
#include <libs/common/types.h>

// アルファベットを大文字に変換する
static inline int toupper(int ch) {
    return ('a' <= ch && ch <= 'z') ? ch - 0x20 : ch;
}

// 文字が数字かどうかを判定する
static inline bool isdigit(int ch) {
    return '0' <= ch && ch <= '9';
}
