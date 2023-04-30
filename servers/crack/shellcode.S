.text

// 文字を出力するマクロ。引数に文字を指定する。
.macro putc c
    li a1, \c
    sw a1, 0(a0)
.endm

.global shellcode
shellcode:
    li a0, 0x10000000  // シリアルポートのアドレス。ユーザーランドからはアクセスできない。
    putc '\n'          // シリアルポートに文字列「*** exploited! ***」を出力する。
    putc '\n'
    putc '*'
    putc '*'
    putc '*'
    putc ' '
    putc 'e'
    putc 'x'
    putc 'p'
    putc 'l'
    putc 'o'
    putc 'i'
    putc 't'
    putc 'e'
    putc 'd'
    putc '!'
    putc ' '
    putc '*'
    putc '*'
    putc '*'
    putc '\n'
    putc '\n'
1:                      // 無限ループ
    j 1b

    // ここには到達しない。書きかわったことを確認するための文字列。
    .ascii "\\\\\\ OVERWRITTEN BY CRACK PROGRAM //////"
