#include <libs/common/backtrace.h>
#include <libs/common/print.h>

// シンボルテーブルの中から指定されたアドレスに最も近いシンボルを探す。
struct symbol *find_symbol(vaddr_t addr) {
    ASSERT(__symbol_table.magic == SYMBOL_TABLE_MAGIC);

    // 二分探索でシンボルを探す。
    int32_t l = -1;
    int32_t r = __symbol_table.num_symbols;
    while (r - l > 1) {
        int32_t mid = (l + r) / 2;
        if (addr >= __symbol_table.symbols[mid].addr) {
            l = mid;
        } else {
            r = mid;
        }
    }

    return (l < 0) ? NULL : &__symbol_table.symbols[l];
}
