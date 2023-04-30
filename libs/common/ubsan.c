//
// Undefined Behavior Sanitizer (UBSan) のランタイムライブラリ
//
// UBSanを使うことで、コンパイル時には検出できない未定義動作を実行時に検出する。
// 未定義動作が検出されると以下で定義されているハンドラ関数たちが呼び出される。
//
// https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
//
#include "ubsan.h"
#include "print.h"

static inline void report_ubsan_event(const char *event) {
    PANIC("detected an undefined behavior: %s (ip=%p)", event,
          __builtin_return_address(0));
}

void __ubsan_handle_type_mismatch_v1(struct ubsan_mismatch_data_v1 *data,
                                     vaddr_t ptr) {
    if (!ptr) {
        report_ubsan_event("NULL pointer dereference");
    } else if (data->align && !IS_ALIGNED(ptr, 1 << data->align)) {
        WARN("UBSan: %s:%d: pointer %p is not aligned to %d", data->loc.file,
             data->loc.line, ptr, 1 << data->align);
    } else {
        PANIC("UBSan: %s:%d: pointer %p is not large enough for %s",
              data->loc.file, data->loc.line, ptr, data->type->name);
    }
}

void __ubsan_handle_add_overflow(void) {
    report_ubsan_event("add_overflow");
}
void __ubsan_handle_sub_overflow(void) {
    report_ubsan_event("sub overflow");
}
void __ubsan_handle_mul_overflow(void) {
    report_ubsan_event("mul overflow");
}
void __ubsan_handle_divrem_overflow(void) {
    report_ubsan_event("divrem overflow");
}
void __ubsan_handle_negate_overflow(void) {
    report_ubsan_event("negate overflow");
}
void __ubsan_handle_float_cast_overflow(void) {
    report_ubsan_event("float cast overflow");
}
void __ubsan_handle_pointer_overflow(void) {
    report_ubsan_event("pointer overflow");
}
void __ubsan_handle_out_of_bounds(void) {
    report_ubsan_event("out of bounds");
}
void __ubsan_handle_shift_out_of_bounds(void) {
    report_ubsan_event("shift out of bounds");
}
void __ubsan_handle_builtin_unreachable(void) {
    report_ubsan_event("builtin unreachable");
}
void __ubsan_handle_invalid_builtin(void) {
    report_ubsan_event("invalid builtin");
}
