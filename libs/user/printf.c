#include <libs/common/string.h>
#include <libs/common/vprintf.h>
#include <libs/user/syscall.h>

static char printbuf[256];  // 文字出力リングバッファ
static int head = 0;        // リングバッファの書き込み位置
static int tail = 0;        // リングバッファの読み込み位置

// カーネルのシリアル出力システムコールを呼び出す。
static void write_all(const char *str, size_t len) {
    while (len > 0) {
        int written_len = sys_serial_write(str, len);
        if (written_len <= 0) {
            // カーネルがエラーを返した。
            break;
        }

        len -= written_len;
        str += written_len;
    }
}

// リングバッファの内容をカーネルに出力する。
void printf_flush(void) {
    if (tail < head) {
        write_all(&printbuf[tail], head - tail);
        tail = head;
    } else if (tail > head) {
        // 一旦ひとつの配列にコピーしてから出力する。一度に出力しないとマルチプロセッサ環境の
        // 場合に表示が乱れる。
        char tmp[sizeof(printbuf)];
        memcpy(tmp, &printbuf[tail], sizeof(printbuf) - tail);
        memcpy(&tmp[sizeof(printbuf) - tail], &printbuf[0], head);

        write_all(tmp, sizeof(printbuf) - tail + head);
        tail = head;
    }
}

// 文字を出力する
void printchar(char ch) {
    // リングバッファに文字を追加する
    printbuf[head] = ch;
    head = (head + 1) % sizeof(printbuf);

    // リングバッファが満杯か改行文字が来たら実際に出力する
    if ((head < tail && head + 1 == tail) || ch == '\n') {
        printf_flush();
    }
}

// 文字列を出力する
void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    vprintf(fmt, vargs);
    va_end(vargs);
}

// パニック関数が呼ばれ、パニックメッセージを出力する前に呼ばれる。
void panic_before_hook(void) {
    // 何もしない
}

// パニック関数が呼ばれ、パニックメッセージを出力した後に呼ばれる。
__noreturn void panic_after_hook(void) {
    sys_task_exit();
}
