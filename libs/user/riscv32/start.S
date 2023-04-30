// ユーザーモードのエントリーポイント
.align 4
.global start
start:
    mv fp, zero       // フレームポインタをゼロに初期化することで、スタックトレースがここで
                      // 停止するようにする
    la sp, __stack    // スタックポインタをスタックの最上位に設定する

    jal hinaos_init   // userライブラリの初期化
    jal main          // ユーザープログラムのエントリーポイント (main関数)

    jal sys_task_exit // main関数から戻ってきたらタスクを終了する
