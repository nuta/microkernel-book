#pragma once
#include <libs/common/message.h>
#include <libs/common/types.h>

// HinaVMの命令セット
//
// 引数の意味は以下の通り:
//
//   - PC: プログラムカウンタ
//   - R[A]: レジスタA
//   - A, B, C: レジスタ番号
//   - L: ラベル番号
//   - imm: 即値
//   - MSG: メッセージバッファ (CURRENT_TASK->m)
//   - MSG[x:y]: メッセージバッファの [x, y) バイト目
enum hinavm_opcode {
    //
    // 命令           引数     説明
    //
    HINAVM_ILLEGAL,    //           (無効な命令)
    HINAVM_NOP,        //           何もしない
    HINAVM_MOVI,       //  A imm    R[A] = imm
    HINAVM_MOV,        //  A B      R[A] = R[B]
    HINAVM_ADD,        //  A B C    R[A] = R[B] + R[C]
    HINAVM_SUB,        //  A B C    R[A] = R[B] - R[C]
    HINAVM_MUL,        //  A B C    R[A] = R[B] * R[C]
    HINAVM_DIV,        //  A B C    R[A] = R[B] / R[C]
    HINAVM_MOD,        //  A B C    R[A] = R[B] % R[C]
    HINAVM_SHL,        //  A B C    R[A] = R[B] << R[C]
    HINAVM_SHR,        //  A B C    R[A] = R[B] >> R[C]
    HINAVM_AND,        //  A B C    R[A] = R[B] & R[C]
    HINAVM_OR,         //  A B C    R[A] = R[B] | R[C]
    HINAVM_XOR,        //  A B C    R[A] = R[B] ^ R[C]
    HINAVM_EQ,         //  A B C    R[A] = (R[B] == R[C]) ? 1 : 0
    HINAVM_NE,         //  A B C    R[A] = (R[B] != R[C]) ? 1 : 0
    HINAVM_LT,         //  A B C    R[A] = (R[B] < R[C])  ? 1 : 0
    HINAVM_LE,         //  A B C    R[A] = (R[B] <= R[C]) ? 1 : 0
    HINAVM_LABEL,      //  L        ラベル L の定義
    HINAVM_JMP,        //  L        PC = L
    HINAVM_JMP_IF,     //  L A      PC = (R[A] == 0) ? pc + 1 : L
    HINAVM_LDM8,       //  A imm    R[A] = MSG[imm:imm+1]
    HINAVM_LDM16,      //  A imm    R[A] = MSG[imm:imm+2]
    HINAVM_LDM32,      //  A imm    R[A] = MSG[imm:imm+4]
    HINAVM_STM8,       //  A imm    MSG[imm:imm+1] = R[A]
    HINAVM_STM16,      //  A imm    MSG[imm:imm+2] = R[A]
    HINAVM_STM32,      //  A imm    MSG[imm:imm+4] = R[A]
    HINAVM_SEND,       //  A B      R[A] = ipc_send(dst = R[B], message = M)
    HINAVM_REPLY,      //  A B      R[A] = ipc_reply(dst = R[B], message = M)
    HINAVM_RECV,       //  A B      R[A] = ipc_recv(src = R[B], message = M)
    HINAVM_PRINT,      //  A        R[A] を32ビット符号付き整数として出力する (10進数)
    HINAVM_PRINT_HEX,  //  A        R[A] を32ビット符号なし整数として出力する (16進数)
    HINAVM_EXIT,       //           タスクを終了する
};

// HinaVMにおける機械語の構造
union hinavm_inst {
    // 基本的な命令の構造
    struct {
        uint32_t opcode : 8;
        uint32_t raw : 24;
    };
    // 即値を読み込む命令 (MOVI)
    struct {
        uint32_t opcode : 8;
        uint32_t a : 4;
        uint32_t imm : 20;
    } movi;
    // レジスタの値をコピーする命令 (MOV)
    struct {
        uint32_t opcode : 8;
        uint32_t a : 4;
        uint32_t b : 4;
        uint32_t unused : 16;
    } mov;
    // 二項演算命令 (ADD, SUB, MUL, DIV, ...)
    struct {
        uint32_t opcode : 8;
        uint32_t a : 4;
        uint32_t b : 4;
        uint32_t c : 4;
        uint32_t unused : 12;
    } binop;
    // ラベル定義命令 (LABEL)
    struct {
        uint32_t opcode : 8;
        uint32_t l : 4;
        uint32_t unused : 20;
    } labeldef;
    // ジャンプ命令 (JMP, JMP_IF)
    struct {
        uint32_t opcode : 8;
        uint32_t l : 4;
        uint32_t a : 4;
        uint32_t unused : 16;
    } jmp;
    // メッセージバッファの読み書き命令 (LDM8, LDM16, ...)
    struct {
        uint32_t opcode : 8;
        uint32_t a : 4;
        uint32_t imm : 20;
    } msg;
    // デバッグ用出力命令 (PRINT)
    struct {
        uint32_t opcode : 8;
        uint32_t a : 4;
        uint32_t unused : 20;
    } print;
    // メッセージパッシング命令 (SEND, REPLY, RECV)
    struct {
        uint32_t opcode : 8;
        uint32_t a : 4;
        uint32_t b : 4;
        uint32_t unused : 16;
    } ipc;
};

typedef union hinavm_inst hinavm_inst_t;

STATIC_ASSERT(sizeof(hinavm_inst_t) == sizeof(uint32_t),
              "invalid size of struct hinavm_inst");

// ハンドアセンブル用マクロ
#define I_MOVI(a_, imm_)                                                       \
    {                                                                          \
        .movi = {.opcode = HINAVM_MOVI, .a = (a_), .imm = (imm_) }             \
    }
#define I_MOV(a_, b_)                                                          \
    {                                                                          \
        .mov = {.opcode = HINAVM_MOV, .a = (a_), .b = (b_) }                   \
    }
#define I_ADD(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_ADD, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_SUB(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_SUB, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_MUL(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_MUL, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_DIV(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_DIV, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_MOD(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_MOD, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_SHR(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_SHR, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_SHL(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_SHL, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_AND(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_AND, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_OR(a_, b_, c_)                                                       \
    {                                                                          \
        .binop = {.opcode = HINAVM_OR, .a = (a_), .b = (b_), .c = (c_) }       \
    }
#define I_XOR(a_, b_, c_)                                                      \
    {                                                                          \
        .binop = {.opcode = HINAVM_XOR, .a = (a_), .b = (b_), .c = (c_) }      \
    }
#define I_EQ(a_, b_, c_)                                                       \
    {                                                                          \
        .binop = {.opcode = HINAVM_EQ, .a = (a_), .b = (b_), .c = (c_) }       \
    }
#define I_NE(a_, b_, c_)                                                       \
    {                                                                          \
        .binop = {.opcode = HINAVM_NE, .a = (a_), .b = (b_), .c = (c_) }       \
    }
#define I_LT(a_, b_, c_)                                                       \
    {                                                                          \
        .binop = {.opcode = HINAVM_LT, .a = (a_), .b = (b_), .c = (c_) }       \
    }
#define I_LE(a_, b_, c_)                                                       \
    {                                                                          \
        .binop = {.opcode = HINAVM_LE, .a = (a_), .b = (b_), .c = (c_) }       \
    }
#define I_LABEL(l_)                                                            \
    {                                                                          \
        .labeldef = {.opcode = HINAVM_LABEL, .l = (l_) }                       \
    }
#define I_JMP(l_)                                                              \
    {                                                                          \
        .jmp = {.opcode = HINAVM_JMP, .l = (l_) }                              \
    }
#define I_JMP_IF(l_, a_)                                                       \
    {                                                                          \
        .jmp = {.opcode = HINAVM_JMP_IF, .l = (l_), .a = (a_) }                \
    }
#define I_LDM8(a_, imm_)                                                       \
    {                                                                          \
        .msg = {.opcode = HINAVM_LDM8, .a = (a_), .imm = (imm_) }              \
    }
#define I_LDM16(a_, imm_)                                                      \
    {                                                                          \
        .msg = {.opcode = HINAVM_LDM16, .a = (a_), .imm = (imm_) }             \
    }
#define I_LDM32(a_, imm_)                                                      \
    {                                                                          \
        .msg = {.opcode = HINAVM_LDM32, .a = (a_), .imm = (imm_) }             \
    }
#define I_STM8(a_, imm_)                                                       \
    {                                                                          \
        .msg = {.opcode = HINAVM_STM8, .a = (a_), .imm = (imm_) }              \
    }
#define I_STM16(a_, imm_)                                                      \
    {                                                                          \
        .msg = {.opcode = HINAVM_STM16, .a = (a_), .imm = (imm_) }             \
    }
#define I_STM32(a_, imm_)                                                      \
    {                                                                          \
        .msg = {.opcode = HINAVM_STM32, .a = (a_), .imm = (imm_) }             \
    }
#define I_PRINT(a_)                                                            \
    {                                                                          \
        .print = {.opcode = HINAVM_PRINT, .a = (a_) }                          \
    }
#define I_PRINT_HEX(a_)                                                        \
    {                                                                          \
        .print = {.opcode = HINAVM_PRINT_HEX, .a = (a_) }                      \
    }
#define I_SEND(ret_, dst_)                                                     \
    {                                                                          \
        .ipc = {.opcode = HINAVM_SEND, .a = (ret_), .b = (dst_) }              \
    }
#define I_REPLY(ret_, dst_)                                                    \
    {                                                                          \
        .ipc = {.opcode = HINAVM_REPLY, .a = (ret_), .b = (dst_) }             \
    }
#define I_RECV(ret_, src_)                                                     \
    {                                                                          \
        .ipc = {.opcode = HINAVM_RECV, .a = (ret_), .b = (src_) }              \
    }
#define I_EXIT()                                                               \
    { .opcode = HINAVM_EXIT }
#define I_NOP()                                                                \
    { .opcode = HINAVM_NOP }

// レジスタ番号
#define R0  0
#define R1  1
#define R2  2
#define R3  3
#define R4  4
#define R5  5
#define R6  6
#define R7  7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15

// ラベル番号
#define LABEL_0  0
#define LABEL_1  1
#define LABEL_2  2
#define LABEL_3  3
#define LABEL_4  4
#define LABEL_5  5
#define LABEL_6  6
#define LABEL_7  7
#define LABEL_8  8
#define LABEL_9  9
#define LABEL_10 10
#define LABEL_11 11
#define LABEL_12 12
#define LABEL_13 13
#define LABEL_14 14
#define LABEL_15 15
