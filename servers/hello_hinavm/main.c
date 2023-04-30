#include <libs/common/hinavm_types.h>
#include <libs/common/print.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>
#include <libs/user/task.h>

#define M(field) offsetof(struct message, field)

union hinavm_inst program[] = {
    I_LABEL(LABEL_0),                  // ラベル0: メインループ
    I_MOVI(R0, IPC_ANY),               // RECV命令に渡す受信元を設定する
    I_RECV(R0, R0),                    // メッセージを受信する
    I_NE(R0, R0, 0),                   // エラーが発生したか？
    I_JMP_IF(LABEL_1, R0),             // エラーが発生したら ラベル1 にジャンプ
    I_LDM32(R0, M(ping.value)),        // 受信したメッセージの値を読み込む
    I_LDM16(R1, M(src)),               // 受信したメッセージの送信元を読み込む
    I_PRINT(R0),                       // 受信したメッセージの値を出力する
    I_MOVI(R0, PING_REPLY_MSG),        // 返信メッセージの種類をセットする
    I_STM32(M(type), R0),              // 返信メッセージの種類を書き込む
    I_MOVI(R0, 42),                    // 返信メッセージの値をセットする
    I_STM32(R0, M(ping_reply.value)),  // 返信メッセージの値を書き込む
    I_REPLY(R0, R1),                   // 返信する
    I_JMP(LABEL_0),                    // ループ

    I_LABEL(LABEL_1),                  // ラベル1: エラー時処理
    I_PRINT(R0),                       // エラー値を出力する
    I_JMP(LABEL_0),                    // メインループに戻る
};

void main(void) {
    // HinaVMタスクを生成する
    task_t server =
        sys_hinavm("hinavm_server", (hinavm_inst_t *) program,
                   sizeof(program) / sizeof(program[0]), task_self());
    ASSERT_OK(server);

    // HinaVMタスクにメッセージを送信する
    struct message m;
    m.type = PING_MSG;
    m.ping.value = 123;
    ASSERT_OK(ipc_call(server, &m));

    // HinaVMタスクからの返信メッセージの内容を表示する
    INFO("reply type: %s", msgtype2str(m.type));
    INFO("reply value: %d", m.ping_reply.value);

    // HinaVMタスクを終了する
    ASSERT_OK(sys_task_destroy(server));
}
