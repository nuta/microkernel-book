#pragma once
#include <libs/common/types.h>

void riscv32_task_switch(uint32_t *prev_sp, uint32_t *next_sp);
void riscv32_kernel_entry_trampoline(void);
void riscv32_user_entry_trampoline(void);
