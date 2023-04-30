// printf関数のラッパーマクロ。カーネル・ユーザーランド両方で利用できるように定義されている。
#pragma once
#include <libs/common/backtrace.h>
#include <libs/common/error.h>
#include <libs/common/types.h>
#include <libs/common/vprintf.h>

// カーネルまたはuserライブラリのどちらかで定義されている。
void printf(const char *fmt, ...);
void printf_flush(void);
const char *__program_name(void);
void panic_before_hook(void);
__noreturn void panic_after_hook(void);

#define PRINT_NL "\r\n"

// ANSI エスケープシーケンス (SGR).
#define SGR_ERR   "\e[1;91m"  // 赤色 + 太字
#define SGR_WARN  "\e[1;33m"  // 黄色 + 太字
#define SGR_INFO  "\e[0;96m"  // シアン (青緑色)
#define SGR_DEBUG "\e[1;95m"  // マゼンタ (紫色) + 太字
#define SGR_RESET "\e[0m"     // 色をリセット

// トレースメッセージを出力する。printf関数と同じ引数を取る。
#define TRACE(fmt, ...)                                                        \
    do {                                                                       \
        printf("[%s] " fmt PRINT_NL, __program_name(), ##__VA_ARGS__);         \
    } while (0)

// デバッグメッセージを出力する。printf関数と同じ引数を取る。
#define DBG(fmt, ...)                                                          \
    do {                                                                       \
        printf(SGR_DEBUG "[%s] " fmt SGR_RESET PRINT_NL, __program_name(),     \
               ##__VA_ARGS__);                                                 \
    } while (0)

// メッセージを出力する。printf関数と同じ引数を取る。
#define INFO(fmt, ...)                                                         \
    do {                                                                       \
        printf(SGR_INFO "[%s] " fmt SGR_RESET PRINT_NL, __program_name(),      \
               ##__VA_ARGS__);                                                 \
    } while (0)

// 警告メッセージを出力する。printf関数と同じ引数を取る。
#define WARN(fmt, ...)                                                         \
    do {                                                                       \
        printf(SGR_WARN "[%s] WARN: " fmt SGR_RESET PRINT_NL,                  \
               __program_name(), ##__VA_ARGS__);                               \
    } while (0)

// エラーメッセージを出力する。printf関数と同じ引数を取る。
#define ERROR(fmt, ...)                                                        \
    do {                                                                       \
        printf(SGR_ERR "[%s] ERROR: " fmt SGR_RESET PRINT_NL,                  \
               __program_name(), ##__VA_ARGS__);                               \
    } while (0)

// バイナリデータを16進数で出力する。ptrはバイト列へのポインタ、lenは表示するバイト数。
#define HEXDUMP(ptr, len)                                                      \
    do {                                                                       \
        uint8_t *__ptr = (uint8_t *) (ptr);                                    \
        size_t __len = len;                                                    \
        printf("[%s] %04x: ", __program_name(), 0);                            \
        size_t i;                                                              \
        for (i = 0; i < __len; i++) {                                          \
            if (i > 0 && i % 16 == 0) {                                        \
                printf(PRINT_NL);                                              \
                printf("[%s] %04x: ", __program_name(), i);                    \
            }                                                                  \
                                                                               \
            printf("%02x ", __ptr[i]);                                         \
        }                                                                      \
                                                                               \
        printf(PRINT_NL);                                                      \
    } while (0)

// アサーション。exprが偽ならエラーメッセージを出力してプログラムを終了する。
#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            panic_before_hook();                                               \
            printf(SGR_ERR                                                     \
                   "[%s] %s:%d: ASSERTION FAILURE: %s" SGR_RESET PRINT_NL,     \
                   __program_name(), __FILE__, __LINE__, #expr);               \
            backtrace();                                                       \
            panic_after_hook();                                                \
            __builtin_unreachable();                                           \
        }                                                                      \
    } while (0)

// デバッグ用アサーション。デバッグビルド時のみ有効になる。
#ifdef DEBUG_BUILD
#    define DEBUG_ASSERT(expr) ASSERT(expr)
#else
#    define DEBUG_ASSERT(expr) ((void) (expr))
#endif

// アサーション。exprがエラー値を返したらエラーメッセージを出力してプログラムを終了する。
#define ASSERT_OK(expr)                                                        \
    do {                                                                       \
        __typeof__(expr) __expr = (expr);                                      \
        if (IS_ERROR(__expr)) {                                                \
            panic_before_hook();                                               \
            printf(SGR_ERR "[%s] %s:%d: unexpected error (%s)\n" SGR_RESET,    \
                   __program_name(), __FILE__, __LINE__, err2str(__expr));     \
            backtrace();                                                       \
            panic_after_hook();                                                \
            __builtin_unreachable();                                           \
        }                                                                      \
    } while (0)

// 警告メッセージをスタックトレース付きで出力する。printf関数と同じ引数を取る。
#define OOPS(fmt, ...)                                                         \
    do {                                                                       \
        printf(SGR_WARN "[%s] Oops: " fmt SGR_RESET PRINT_NL,                  \
               __program_name(), ##__VA_ARGS__);                               \
        backtrace();                                                           \
    } while (0)

// アサーション。exprがエラー値を返したら警告メッセージをスタックトレース付きで出力する。
#define OOPS_OK(expr)                                                          \
    do {                                                                       \
        __typeof__(expr) __expr = (expr);                                      \
        if (IS_ERROR(__expr)) {                                                \
            printf(SGR_WARN "[%s] Oops: Unexpected error (%s)\n" SGR_RESET,    \
                   __program_name(), err2str(__expr));                         \
            backtrace();                                                       \
        }                                                                      \
    } while (0)

// エラーメッセージを出力してプログラムを終了する。printf関数と同じ引数を取る。
#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        panic_before_hook();                                                   \
        printf(SGR_ERR "[%s] PANIC: " fmt SGR_RESET PRINT_NL,                  \
               __program_name(), ##__VA_ARGS__);                               \
        backtrace();                                                           \
        panic_after_hook();                                                    \
        __builtin_unreachable();                                               \
    } while (0)

// まだ実装されていないことを示すエラーメッセージを出力してプログラムを終了する。
// printf関数と同じ引数を取る。
#define NYI()                                                                  \
    do {                                                                       \
        panic_before_hook();                                                   \
        printf(SGR_ERR "[%s] %s(): not yet ymplemented: %s:%d\n SGR_RESET",    \
               __program_name(), __func__, __FILE__, __LINE__);                \
        panic_after_hook();                                                    \
        __builtin_unreachable();                                               \
    } while (0)

// 到達すべきでないコードに辿り着いたことを示すエラーメッセージを出力してプログラムを終了する。
// printf関数と同じ引数を取る。
#define UNREACHABLE()                                                          \
    do {                                                                       \
        panic_before_hook();                                                   \
        printf(SGR_ERR "Unreachable at %s:%d (%s)\n" SGR_RESET, __FILE__,      \
               __LINE__, __func__);                                            \
        backtrace();                                                           \
        panic_after_hook();                                                    \
        __builtin_unreachable();                                               \
    } while (0)
