#pragma once
#include <libs/common/hinavm_types.h>

#define HINAVM_INSTS_MAX 128

// HinaVMを起動するために必要な情報 (HinaVMプログラム)
struct hinavm {
    hinavm_inst_t insts[HINAVM_INSTS_MAX];  // 命令列
    uint32_t num_insts;                     // 命令数
};

__noreturn void hinavm_run(struct hinavm *hinavm);
