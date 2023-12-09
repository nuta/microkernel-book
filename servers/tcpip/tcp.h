#pragma once
#include "ipv4.h"
#include "mbuf.h"
#include <libs/common/list.h>

// TCP通信の管理構造体の最大数
#define TCP_PCBS_MAX 512
// 再送タイムアウトの初期値
#define TCP_TX_INITIAL_TIMEOUT 500
// 再送タイムアウトの最大値
#define TCP_TX_MAX_TIMEOUT 3000
// 受信バッファのサイズ (初期ウィンドウサイズ)
#define TCP_RX_BUF_SIZE 8192

// TCPコネクションの状態 (参考: TCPの状態遷移図)
//
// 簡単のため、このTCP実装は素朴なHTTPクライアントが動く程度の状態しか実装していない。
// パッシブオープンやアクティブクローズなどを実装する場合には、より多くの状態が登場する。
enum tcp_state {
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_RECVED,
    TCP_STATE_SYN_SENT,     // SYNを送信し、SYN+ACKを待っている状態
    TCP_STATE_ESTABLISHED,  // コネクションを確立した状態
    TCP_STATE_LAST_ACK,
    TCP_STATE_CLOSED,       // コネクションが閉じられた状態
};

// 送信待ちフラグ
enum tcp_pending_flag {
    TCP_PEND_ACK = 1 << 0,
    TCP_PEND_FIN = 1 << 1,
    TCP_PEND_SYN = 1 << 2,
};

// TCP通信の管理構造体 (PCB: Protocol Control Block)
struct tcp_pcb {
    bool in_use;               // 使用中かどうか
    enum tcp_state state;      // コネクションの状態
    uint32_t pending_flags;    // 送信する必要があるフラグ
    uint32_t next_seqno;       // 次に送信すべきシーケンス番号
    uint32_t last_seqno;       // 最後に送信したシーケンス番号
    uint32_t last_ack;         // 最後に受信したシーケンス番号 + 1
    uint32_t local_winsize;    // 送信ウィンドウサイズ
    uint32_t remote_winsize;   // 受信ウィンドウサイズ
    endpoint_t local;          // ソケットに紐付けられたIPアドレスとポート番号
    endpoint_t remote;         // 相手のIPアドレスとポート番号
    mbuf_t rx_buf;             // 受信バッファ
    mbuf_t tx_buf;             // 送信バッファ
    unsigned num_retransmits;  // 再送回数
    int retransmit_at;         // 次に再送すべき時刻
    list_elem_t next;          // 次の要素へのポインタ
    void *arg;                 // コールバック関数に渡す引数
    struct tcp_pcb  *parent;
};

// TCPヘッダのフラグ
enum tcp_header_flag {
    TCP_FIN = 1 << 0,
    TCP_SYN = 1 << 1,
    TCP_RST = 1 << 2,
    TCP_PSH = 1 << 3,
    TCP_ACK = 1 << 4,
};

// TCPヘッダ
struct tcp_header {
    uint16_t src_port;   // 送信元ポート番号
    uint16_t dst_port;   // 宛先ポート番号
    uint32_t seqno;      // シーケンス番号
    uint32_t ackno;      // 確認応答番号
    uint8_t off_and_ns;  // ヘッダ長など
    uint8_t flags;       // フラグ
    uint16_t win_size;   // ウィンドウサイズ
    uint16_t checksum;   // チェックサム
    uint16_t urgent;     // 緊急ポインタ
} __packed;

struct tcp_pcb *tcp_new(void *arg);
error_t tcp_bind(struct tcp_pcb *pcb, ipv4addr_t addr, port_t port);
void tcp_listen(struct tcp_pcb *pcb);
error_t tcp_connect(struct tcp_pcb *sock, ipv4addr_t dst_addr, port_t dst_port);
void tcp_close(struct tcp_pcb *sock);
void tcp_write(struct tcp_pcb *sock, const void *data, size_t len);
size_t tcp_read(struct tcp_pcb *sock, void *buf, size_t buf_len);
void tcp_receive(ipv4addr_t dst, ipv4addr_t src, mbuf_t pkt);
void tcp_flush(void);
