// 実行コンテキストを切り替える
//
// void riscv32_task_switch(uint32_t *prev_sp, uint32_t *next_sp);
//                                    ^^^^^^^            ^^^^^^^
//                                    a0レジスタ          a1レジスタ
.align 4
.global riscv32_task_switch
riscv32_task_switch:
    addi sp, sp, -13 * 4 // 13個のレジスタを保存するためのスペースを確保

    // 呼び出し先保存 (callee-saved) レジスタをスタックに保存
    sw ra,  0  * 4(sp)  // 戻り先アドレス (この関数の呼び出し元)
    sw s0,  1  * 4(sp)
    sw s1,  2  * 4(sp)
    sw s2,  3  * 4(sp)
    sw s3,  4  * 4(sp)
    sw s4,  5  * 4(sp)
    sw s5,  6  * 4(sp)
    sw s6,  7  * 4(sp)
    sw s7,  8  * 4(sp)
    sw s8,  9  * 4(sp)
    sw s9,  10 * 4(sp)
    sw s10, 11 * 4(sp)
    sw s11, 12 * 4(sp)

    sw sp, (a0)  // 実行中タスクのスタックポインタを保存
    lw sp, (a1)  // 次に実行するタスクのスタックポインタを復元

    // レジスタをスタックから復元
    lw ra,  0  * 4(sp)  // 戻り先アドレス (この関数の呼び出し元)
    lw s0,  1  * 4(sp)
    lw s1,  2  * 4(sp)
    lw s2,  3  * 4(sp)
    lw s3,  4  * 4(sp)
    lw s4,  5  * 4(sp)
    lw s5,  6  * 4(sp)
    lw s6,  7  * 4(sp)
    lw s7,  8  * 4(sp)
    lw s8,  9  * 4(sp)
    lw s9,  10 * 4(sp)
    lw s10, 11 * 4(sp)
    lw s11, 12 * 4(sp)

    addi sp, sp, 13 * 4  // 12個のレジスタを取り出したので、スタックポインタを更新
    ret                  // 次に実行するタスクの実行を再開する

// カーネルタスクのエントリポイント
.align 4
.global riscv32_kernel_entry_trampoline
riscv32_kernel_entry_trampoline:
    // スタックから引数を取り出して、エントリーポイントにジャンプする
    lw a0, 0 * 4(sp)  // a0
    lw a1, 1 * 4(sp)  // ip
    add sp, sp, 2 * 4 // 取り出した引数だけ解放する
    jalr a1

    // 戻ってくるべきではない
1:
    j 1b

// ユーザータスクのエントリポイント
.align 4
.global riscv32_user_entry_trampoline
riscv32_user_entry_trampoline:
    // スタックから引数を取り出して、riscv32_user_entry関数にジャンプする
    lw a0, 0 * 4(sp) // ip
    j riscv32_user_entry
