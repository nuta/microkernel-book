// void arch_memcpy_from_user(void *dst, __user const void *src, size_t len);
//                                  ^^^                     ^^^         ^^^
//                                  a0レジスタ          a1レジスタ      a2レジスタ
.global arch_memcpy_from_user
.global riscv32_usercopy1
arch_memcpy_from_user:
    beqz a2, 2f        // a2 (コピー長) がゼロならラベル「2」にジャンプ
1:
riscv32_usercopy1:
    lb a3, 0(a1)      // a1レジスタの指すアドレス (ユーザーポインタ) から1バイト読み込む

    sb a3, 0(a0)      // a0レジスタの指すアドレス (カーネルポインタ) に1バイト書き込む
    addi a1, a1, 1    // a1レジスタ (ユーザーポインタ) を1バイト進める
    addi a0, a0, 1    // a0レジスタ (カーネルポインタ) を1バイト進める
    addi a2, a2, -1   // a2レジスタの値を1減らす
    bnez a2, 1b       // a2レジスタの値がゼロでなければラベル「1」にジャンプ
2:
    ret                // 関数から戻る

// void arch_memcpy_to_user(__user void *dst, const void *src, size_t len);
//                                       ^^^              ^^^         ^^^
//                                  a0レジスタ        a1レジスタ       a2レジスタ
.global arch_memcpy_to_user
.global riscv32_usercopy2
arch_memcpy_to_user:
    beqz a2, 2f        // a2 (コピー長) がゼロならラベル「2」にジャンプ
1:
    lb a3, 0(a1)       // a1レジスタの指すアドレス (カーネルポインタ) から1バイト読み込む

riscv32_usercopy2:
    sb a3, 0(a0)       // a0レジスタの指すアドレス (ユーザーポインタ) に1バイト書き込む

    addi a0, a0, 1     // a0レジスタ (ユーザーポインタ) を1バイト進める
    addi a1, a1, 1     // a1レジスタ (カーネルポインタ) を1バイト進める
    addi a2, a2, -1    // a2レジスタの値を1減らす
    bnez a2, 1b        // a2レジスタの値がゼロでなければラベル「1」にジャンプ
2:
    ret                // 関数から戻る
