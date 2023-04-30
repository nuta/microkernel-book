#include <libs/common/print.h>
#include <libs/common/types.h>

// RISC-Vのスタックフレーム
struct stack_frame {
    uint32_t fp;  // 呼び出し元のスタックフレーム
    uint32_t ra;  // 呼び出し元のアドレス
} __packed;

// 現在のスタックフレームのアドレスを取得する。
static uint32_t read_fp(void) {
    uint32_t fp;
    __asm__ __volatile__("mv %0, fp" : "=r"(fp));
    return fp;
}

/// スタックフレームを辿ってバックトレースを表示する。
void backtrace(void) {
    uint32_t fp = read_fp();
    if (!fp) {
        WARN("backtrace: fp is NULL");
        return;
    }

    for (int i = 0; fp && i < BACKTRACE_MAX; i++) {
        struct stack_frame *frame =
            (struct stack_frame *) (fp - sizeof(*frame));

        // 呼び出し元のアドレスからシンボルを探す。
        const struct symbol *symbol = find_symbol(frame->ra);
        const char *name;
        size_t offset;
        if (!symbol) {
            WARN("    #%d: %p (invalid address)", i, frame->ra);
            return;
        }

        name = symbol->name;
        offset = frame->ra - symbol->addr;
        WARN("    #%d: %p %s()+0x%x", i, frame->ra, name, offset);
        fp = frame->fp;
    }
}
