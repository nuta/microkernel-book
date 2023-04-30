#pragma once

#define RAM_SIZE          (128 * 1024 * 1024)  // メモリサイズ (QEMUの-mオプションで指定)
#define NUM_TASKS_MAX     16                   // 最大タスク数
#define NUM_CPUS_MAX      4                    // 最大CPU数
#define TASK_NAME_LEN     16                   // タスクの名前の最大長 (ヌル文字含む)
#define KERNEL_STACK_SIZE (16 * 1024)          // カーネルスタックサイズ
#define VIRTIO_BLK_PADDR  0x10001000           // virtio-blkのMMIOアドレス
#define VIRTIO_NET_PADDR  0x10002000           // virtio-netのMMIOアドレス
#define VIRTIO_NET_IRQ    2                    // virtio-netの割り込み番号
#define TICK_HZ           1000                 // タイマー割り込みの周期
