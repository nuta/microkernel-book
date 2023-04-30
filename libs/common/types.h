#pragma once
#include "buildconfig.h"

typedef char int8_t;                  //  8ビット符号付き整数型
typedef short int16_t;                // 16ビット符号付き整数型
typedef int int32_t;                  // 32ビット符号付き整数型
typedef long long int64_t;            // 64ビット符号付き整数型
typedef unsigned char uint8_t;        // 8ビット符号なし整数型
typedef unsigned short uint16_t;      // 16ビット符号なし整数型
typedef unsigned uint32_t;            // 32ビット符号なし整数型
typedef unsigned long long uint64_t;  // 64ビット符号なし整数型

#if !defined(__LP64__)
// intの最大値
#    define INT_MAX 2147483647
// unsigned intの最大値
#    define UINT_MAX 4294967295U
// 符号付き整数型の最大値
typedef int32_t intmax_t;
// 符号なし整数型の最大値
typedef uint32_t uintmax_t;
#endif

// 真偽値
typedef char bool;
#define true  1
#define false 0

// ヌルポインタ
#define NULL ((void *) 0)

typedef int error_t;               // エラーコードを表す整数型
typedef int task_t;                // タスクID
typedef int handle_t;              // ハンドルID
typedef uint32_t notifications_t;  // 通知を表すビットフィールド

typedef uintmax_t size_t;          // 大きさを表す整数型
typedef long pfn_t;                // 物理ページ番号を表す整数型
typedef uintmax_t paddr_t;         // 物理アドレスを表す整数型
typedef uintmax_t vaddr_t;         // 仮想アドレスを表す整数型
typedef uintmax_t uaddr_t;         // ユーザー空間の仮想アドレスを表す整数型
typedef uintmax_t uintptr_t;       // ポインタが指すアドレスを格納する整数型
typedef uintmax_t offset_t;        // オフセットを表す整数型

// 構造体の属性: パディングを挿入しない
#define __packed __attribute__((packed))
// 関数の属性: この関数は戻らない
#define __noreturn __attribute__((noreturn))
// 関数の属性: この関数の戻り値は必ずチェックすべき
#define __mustuse __attribute__((warn_unused_result))
// 変数の属性: 指定されたアライメントでアラインされていることを保証する
#define __aligned(aligned_to) __attribute__((aligned(aligned_to)))
// ポインタの属性: ユーザーから渡されたポインタであることを示す。このポインタに対して直接アクセス
// するのではなく、memcpy_from_userやmemcpy_to_userなどの安全な関数を使ってアクセスすること。
#define __user __attribute__((noderef, address_space(1)))

// 可変長引数
typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

// exprが0でない場合にコンパイル時にエラーを発生させる
#define STATIC_ASSERT(expr, summary) _Static_assert(expr, summary);
// 構造体のメンバのオフセットを取得する
#define offsetof(type, field) __builtin_offsetof(type, field)
// 与えられた値を指定されたアライメントに合うように切り上げる
#define ALIGN_DOWN(value, align) __builtin_align_down(value, align)
// 与えられた値を指定されたアライメントに合うように切り下げる
#define ALIGN_UP(value, align) __builtin_align_up(value, align)
// 与えられた値が指定されたアライメントに合っているかどうかを判定する
#define IS_ALIGNED(value, align) __builtin_is_aligned(value, align)

// アトミックにポインタの値を読み込む
#define atomic_load(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
// アトミックにポインタの値にビット論理和代入 (|=) を行う
#define atomic_fetch_and_or(ptr, value) __sync_fetch_and_or(ptr, value)
// アトミックにポインタの値にビット論理積代入 (&=) を行う
#define atomic_fetch_and_and(ptr, value) __sync_fetch_and_and(ptr, value)
// Compare-and-Swap (CAS) 操作: ptrの値がoldの場合に、newを代入して真を返す
#define compare_and_swap(ptr, old, new)                                        \
    __sync_bool_compare_and_swap(ptr, old, new)
// メモリバリア
#define full_memory_barrier __sync_synchronize

// aとbのうち大きい方を返す
#define MAX(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a > __b) ? __a : __b;                                               \
    })

// aとbのうち小さい方を返す
#define MIN(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a < __b) ? __a : __b;                                               \
    })

//
// エラーコード
//
#define IS_OK(err)          (!IS_ERROR(err))      // 正常終了かどうかを判定する
#define IS_ERROR(err)       (((long) (err)) < 0)  // エラーかどうかを判定する
#define OK                  0                     // 正常終了
#define ERR_NO_MEMORY       -1                    // メモリ不足
#define ERR_NO_RESOURCES    -2                    // リソースが足りない
#define ERR_ALREADY_EXISTS  -3                    // 既に存在する
#define ERR_ALREADY_USED    -4                    // 既に使われている
#define ERR_ALREADY_DONE    -5                    // 既に完了している
#define ERR_STILL_USED      -6                    // まだ使われている
#define ERR_NOT_FOUND       -7                    // 見つからない
#define ERR_NOT_ALLOWED     -8                    // 許可されていない
#define ERR_NOT_SUPPORTED   -9                    // サポートされていない
#define ERR_UNEXPECTED      -10                   // 予期しない入力値・ケース
#define ERR_INVALID_ARG     -11                   // 無効な引数・入力値
#define ERR_INVALID_TASK    -12                   // 無効なタスクID
#define ERR_INVALID_SYSCALL -13                   // 無効なシステムコール番号
#define ERR_INVALID_PADDR   -14                   // 無効な物理アドレス
#define ERR_INVALID_UADDR   -15                   // 無効なユーザー空間のアドレス
#define ERR_TOO_MANY_TASKS  -16                   // タスクが多すぎる
#define ERR_TOO_LARGE       -17                   // 大きすぎる
#define ERR_TOO_SMALL       -18                   // 小さすぎる
#define ERR_WOULD_BLOCK     -19                   // ブロックしてしまうので中断した
#define ERR_TRY_AGAIN       -20                   // 一時的な失敗: 再試行すると成功するかもしれない
#define ERR_ABORTED         -21                   // 中断された
#define ERR_EMPTY           -22                   // 空である
#define ERR_NOT_EMPTY       -23                   // 空ではない
#define ERR_DEAD_LOCK       -24                   // デッドロックが発生した
#define ERR_NOT_A_FILE      -25                   // ファイルではない
#define ERR_NOT_A_DIR       -26                   // ディレクトリではない
#define ERR_EOF             -27                   // ファイル・データの終端
#define ERR_END             -28                   // 最後のエラーコードでなければならない

// メモリページサイズ
#define PAGE_SIZE 4096
// ページフレーム番号のオフセット
#define PFN_OFFSET 12
// 物理アドレスからページフレーム番号を取り出す
#define PADDR2PFN(paddr) ((paddr) >> PFN_OFFSET)
// ページフレーム番号から物理アドレスを取り出す
#define PFN2PADDR(pfn) (((paddr_t) (pfn)) << PFN_OFFSET)

// カーネルからのメッセージが送信された場合の送信元タスクID
#define FROM_KERNEL -1
// VMサーバ (最初のユーザータスク) のタスクID
#define VM_SERVER 1

// システムコール番号
#define SYS_IPC          1
#define SYS_NOTIFY       2
#define SYS_SERIAL_WRITE 3
#define SYS_SERIAL_READ  4
#define SYS_TASK_CREATE  5
#define SYS_TASK_DESTROY 6
#define SYS_TASK_EXIT    7
#define SYS_TASK_SELF    8
#define SYS_PM_ALLOC     9
#define SYS_VM_MAP       10
#define SYS_VM_UNMAP     11
#define SYS_IRQ_LISTEN   12
#define SYS_IRQ_UNLISTEN 13
#define SYS_TIME         14
#define SYS_UPTIME       15
#define SYS_HINAVM       16
#define SYS_SHUTDOWN     17

// pm_alloc() のフラグ
#define PM_ALLOC_UNINITIALIZED 0         // ゼロクリアされていなくてもよい
#define PM_ALLOC_ZEROED        (1 << 0)  // ゼロクリアされていることを要求する
#define PM_ALLOC_ALIGNED       (1 << 1)  // 要求サイズでアラインされている必要がある

// ページの属性
#define PAGE_READABLE   (1 << 1)  // 読み込み可能
#define PAGE_WRITABLE   (1 << 2)  // 書き込み可能
#define PAGE_EXECUTABLE (1 << 3)  // 実行可能
#define PAGE_USER       (1 << 4)  // ユーザー空間からアクセス可能

// ページフォルトの理由
#define PAGE_FAULT_READ    (1 << 0)  // ページを読み込もうとして発生
#define PAGE_FAULT_WRITE   (1 << 1)  // ページへ書き込もうとして発生
#define PAGE_FAULT_EXEC    (1 << 2)  // ページを実行しようとして発生
#define PAGE_FAULT_USER    (1 << 3)  // ユーザーモードで発生
#define PAGE_FAULT_PRESENT (1 << 4)  // 既に存在するページで発生

// 例外の種類
#define EXP_GRACE_EXIT          1  // タスクが終了した
#define EXP_INVALID_UADDR       2  // マップ不可領域アドレスへのアクセスを試みた
#define EXP_INVALID_PAGER_REPLY 3  // 無効なページャからの返信
#define EXP_ILLEGAL_EXCEPTION   4  // 不正なCPU例外
