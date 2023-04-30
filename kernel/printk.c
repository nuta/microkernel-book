#include "printk.h"
#include "arch.h"
#include "task.h"
#include <libs/common/list.h>
#include <libs/common/string.h>
#include <libs/common/vprintf.h>

// UARTからのデータを待っているタスクのリスト
static list_t serial_readers = LIST_INIT(serial_readers);
// UARTからの入力データのリングバッファと、その読み書き位置
static char input[128];
static int input_rp = 0;
static int input_wp = 0;

// UARTからの割り込みハンドラ
void handle_serial_interrupt(void) {
    while (true) {
        // 1文字読み込む
        int ch = arch_serial_read();
        if (ch == -1) {
            // もう読み込めるデータがない
            break;
        }

        // Ctrl-P: デバッグ情報を出力する
        // https://en.wikipedia.org/wiki/Control_character
        if (ch == 'P' - '@' /* 0x10 */) {
            task_dump();
            continue;
        }

        // バッファに書き込む
        input[input_wp] = ch;
        input_wp = (input_wp + 1) % sizeof(input);

        // バッファが一杯になったら、古いデータを捨てる
        if (input_rp == input_wp) {
            input_rp = (input_rp + 1) % sizeof(input);
        }
    }

    // serial_read関数でブロックしているタスクを再開する
    LIST_FOR_EACH (task, &serial_readers, struct task, waitqueue_next) {
        list_remove(&task->waitqueue_next);
        task_resume(task);
    }
}

// UARTからの入力を読み込む。QEMUは標準入力 (一般にキーボード入力) がUARTに繋がっている。
int serial_read(char *buf, int max_len) {
    int len = 0;
    while (true) {
        // バッファに貯まったデータを全て読み込む
        for (; len < max_len && input_rp != input_wp; len++) {
            char ch = input[input_rp];
            input_rp = (input_rp + 1) % sizeof(input);
            buf[len] = ch;
        }

        // 1文字以上読み込めたら、それを即座に返す
        if (len > 0) {
            break;
        }

        // 1文字も読み込めなかったら、タスクをブロックしてUARTからの割り込みを待つ
        list_push_back(&serial_readers, &CURRENT_TASK->waitqueue_next);
        task_block(CURRENT_TASK);
        task_switch();
    }

    return len;
}

// カーネル内部でのみ使用するputchar実装。UARTに出力する。
void printchar(char ch) {
    arch_serial_write(ch);
}

// カーネル内部でのみ使用するprintf実装。UARTに出力する。
void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    vprintf(fmt, vargs);
    va_end(vargs);
}
