#pragma once
#include <libs/common/types.h>

// スタックカナリー (stack canary) の値。この値が書き換えられていたらスタックオーバーフロー
// が発生していると判断する。
#define STACK_CANARY_VALUE 0xdeadca71

void stack_check(void);
void stack_set_canary(uint32_t sp_bottom);
void stack_reset_current_canary(void);
