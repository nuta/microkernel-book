#include <libs/common/vprintf.h>

// 文字列を出力する。
static void puts(const char *s) {
    while (*s != '\0') {
        printchar(*s);
        s++;
    }
}

// 整数を出力する。n が出力する整数、base が基数。整数を文字列にしたときの長さが width 未満
// であれば文字 pad で埋める。
static void print_uint(uintmax_t n, int base, char pad, int width) {
    char tmp[32];
    int i = sizeof(tmp) - 2;
    do {
        tmp[i] = "0123456789abcdef"[n % base];
        n /= base;
        width--;
        i--;
    } while (n > 0 && i > 0);

    while (width-- > 0) {
        printchar(pad);
    }

    tmp[sizeof(tmp) - 1] = '\0';
    puts(&tmp[i + 1]);
}

//  vprintf関数の実装。printf関数の実装の主要部分。
//
//  %%   - %
//  %c   - 文字
//  %s   - 文字列
//  %d   - 符号付き整数 (10進数)
//  %u   - 符号なし整数 (10進数)
//  %x   - 符号なし整数 (16進数)
//  %p   - ポインタ
//  %pI4 - IPv4アドレス
//
//  以下のオプションをサポートする。
//  l  - long
//  ll - long long
//  h  - short
//  #  - 0x などの接頭辞を付ける
//  0  - 0埋め
void vprintf(const char *fmt, va_list vargs) {
    while (*fmt) {
        if (*fmt != '%') {
            // % でなければそのまま出力する
            printchar(*fmt++);
        } else {
            // 各種オプションをパースする
            int num_len = 1;
            int width = 0;
            char pad = ' ';
            bool alt = false;
            uintmax_t n;
            bool not_attr = false;
            while (*++fmt) {
                switch (*fmt) {
                    case 'l':
                        num_len++;
                        break;
                    case 'h':
                        num_len--;
                        break;
                    case '0':
                        pad = '0';
                        break;
                    case '#':
                        alt = false;
                        break;
                    case '\0':
                        puts("<invalid format>");
                        return;
                    default:
                        not_attr = true;
                }

                if (not_attr) {
                    break;
                }
            }

            // 文字列の最小幅
            if ('1' <= *fmt && *fmt <= '9') {
                width = *fmt - '0';
                fmt++;
            }

            // ここからが各種フォーマットの処理
            switch (*fmt++) {
                case 'd':
                    // 符号付き整数
                    n = (num_len == 3) ? va_arg(vargs, long long)
                                       : va_arg(vargs, int);
                    if ((intmax_t) n < 0) {
                        printchar('-');
                        n = -((intmax_t) n);
                    }
                    print_uint(n, 10, pad, width);
                    break;
                case 'p': {
                    // ポインタ
                    char ch = *fmt++;
                    switch (ch) {
                        case 'I': {
                            // IPv4 address.
                            char ch2 = *fmt++;
                            switch (ch2) {
                                // IPV4 address.
                                case '4': {
                                    uint32_t v4addr = va_arg(vargs, uint32_t);
                                    print_uint((v4addr >> 24) & 0xff, 10, ' ',
                                               0);
                                    printchar('.');
                                    print_uint((v4addr >> 16) & 0xff, 10, ' ',
                                               0);
                                    printchar('.');
                                    print_uint((v4addr >> 8) & 0xff, 10, ' ',
                                               0);
                                    printchar('.');
                                    print_uint(v4addr & 0xff, 10, ' ', 0);
                                    break;
                                }
                                default:
                                    // Invalid format specifier.
                                    puts("%pI");
                                    printchar(ch2);
                                    break;
                            }
                            break;
                        }
                        default:
                            // ポインタ値
                            print_uint((uintmax_t) va_arg(vargs, void *), 16,
                                       '0', sizeof(vaddr_t) * 2);
                            // %p の次の文字を取り出してしまったので戻す。
                            fmt--;
                    }
                    break;
                }
                case 'x':
                    // 符号なし整数 (16進数)
                    alt = true;
                    // fallthrough: 後の処理は %u と同じ
                case 'u':
                    // 符号なし整数 (10進数)
                    n = (num_len == 3) ? va_arg(vargs, unsigned long long)
                                       : va_arg(vargs, unsigned);
                    print_uint(n, alt ? 16 : 10, pad, width);
                    break;
                case 'c':
                    // 文字
                    printchar(va_arg(vargs, int));
                    break;
                case 's': {
                    // 文字列
                    char *s = va_arg(vargs, char *);
                    puts(s ? s : "(null)");
                    break;
                }
                case '%':
                    // %
                    printchar('%');
                    break;
                default:
                    // 未知のフォーマット
                    puts("<invalid format>");
                    return;
            }
        }
    }
}
