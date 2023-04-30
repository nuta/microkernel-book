#pragma once
#include "arch.h"

void kernel_main(struct bootinfo *bootinfo);
void kernel_mp_main(void);

// カーネルイメージに埋め込まれた最初のユーザータスク (VMサーバ) のELFイメージ
extern char __boot_elf[];
