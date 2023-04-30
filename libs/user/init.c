#include <libs/common/print.h>
#include <libs/user/malloc.h>
#include <libs/user/task.h>

// userライブラリの初期化を行う。main関数の前に呼び出される。
void hinaos_init(void) {
    malloc_init();
}
