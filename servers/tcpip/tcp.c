#include "tcp.h"
#include "checksum.h"
#include "ipv4.h"
#include "main.h"
#include <libs/common/endian.h>
#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/syscall.h>

// TCPソケット管理構造体のテーブル
static struct tcp_pcb pcbs[TCP_PCBS_MAX];
// 使用中のTCPソケット管理構造体のリスト
static list_t active_pcbs = LIST_INIT(active_pcbs);

// ローカルのIPアドレスとポート番号からPCBを検索する。
static struct tcp_pcb *tcp_lookup_local(endpoint_t *local_ep) {
    LIST_FOR_EACH (pcb, &active_pcbs, struct tcp_pcb, next) {
        // IPアドレスが一致するか
        if (pcb->local.addr != IPV4_ADDR_UNSPECIFIED
            && pcb->local.addr != local_ep->addr) {
            continue;
        }

        // ポート番号が一致するか
        if (pcb->local.port != local_ep->port) {
            continue;
        }

        return pcb;
    }

    return NULL;
}

// IPアドレスとポート番号の組からPCBを検索する。local_epはローカルの組、remote_epは
// 通信相手の組。同じローカルのポート番号であっても、通信相手が異なれば別のPCBが存在する。
static struct tcp_pcb *tcp_lookup(endpoint_t *local_ep, endpoint_t *remote_ep) {
    LIST_FOR_EACH (pcb, &active_pcbs, struct tcp_pcb, next) {
        // ローカルのIPアドレスが一致するか
        if (pcb->local.addr != IPV4_ADDR_UNSPECIFIED
            && pcb->local.addr != local_ep->addr) {
            continue;
        }

        // ローカルのポート番号が一致するか
        if (pcb->local.port != local_ep->port) {
            continue;
        }

        // 通信相手のIPアドレスが一致するか
        if (pcb->remote.addr != remote_ep->addr) {
            continue;
        }

        // 通信相手のポート番号が一致するか
        if (pcb->remote.port != remote_ep->port) {
            continue;
        }

        return pcb;
    }

    // Look for the listening socket
    return tcp_lookup_local(local_ep);
}

// 新しいPCBを作成する。
struct tcp_pcb *tcp_new(void *arg) {
    // 空いているPCBを探す。
    struct tcp_pcb *pcb = NULL;
    for (int i = 0; i < TCP_PCBS_MAX; i++) {
        if (!pcbs[i].in_use) {
            pcb = &pcbs[i];
            break;
        }
    }

    if (!pcb) {
        return NULL;
    }

    pcb->in_use = true;
    pcb->state = TCP_STATE_CLOSED;
    pcb->pending_flags = 0;
    pcb->next_seqno = 0;
    pcb->last_seqno = 0;
    pcb->last_ack = 0;
    pcb->local_winsize = TCP_RX_BUF_SIZE;
    pcb->local.addr = 0;
    pcb->remote.addr = 0;
    pcb->local.port = 0;
    pcb->remote.port = 0;
    pcb->rx_buf = mbuf_alloc();
    pcb->tx_buf = mbuf_alloc();
    pcb->retransmit_at = 0;
    pcb->num_retransmits = 0;
    pcb->arg = arg;
    pcb->parent = NULL;
    list_elem_init(&pcb->next);
    return pcb;
}

// bind specified ip address & port to pcb
error_t tcp_bind(struct tcp_pcb *pcb, ipv4addr_t addr, port_t port) {
    endpoint_t ep = {.addr = addr, .port = port};

    // return err if port is already in use
    if(tcp_lookup_local(&ep) != NULL) {
        WARN("port %d in use", port);
        return ERR_ALREADY_USED;
    }

    pcb->local = ep;
    
    return OK;
}

void tcp_listen(struct tcp_pcb *pcb) {
    pcb->state = TCP_STATE_LISTEN;
    list_push_back(&active_pcbs, &pcb->next);
}

// TCPコネクションを開く (アクティブオープン)。
error_t tcp_connect(struct tcp_pcb *pcb, ipv4addr_t dst_addr, port_t dst_port) {
    // エフェメラルポート (49152-65535) から空いているポートを探す。
    for (int port = 49152; port <= 65535; port++) {
        endpoint_t ep;
        ep.port = port;
        ep.addr = IPV4_ADDR_UNSPECIFIED;

        if (tcp_lookup_local(&ep) == NULL) {
            // 使われていないポート番号が見つかった。
            memcpy(&pcb->local, &ep, sizeof(pcb->local));
            pcb->remote.addr = dst_addr;
            pcb->remote.port = dst_port;
            pcb->state = TCP_STATE_SYN_SENT;
            pcb->pending_flags |= TCP_PEND_SYN;
            list_push_back(&active_pcbs, &pcb->next);
            return OK;
        }
    }

    WARN("run out of client tcp ports");
    return ERR_TRY_AGAIN;
}

void tcp_destroy(struct tcp_pcb *pcb) {
    mbuf_delete(pcb->rx_buf);
    mbuf_delete(pcb->tx_buf);
    list_remove(&pcb->next);
    pcb->in_use = false;
}

// 送信するデータをバッファに追加する。
void tcp_write(struct tcp_pcb *pcb, const void *data, size_t len) {
    mbuf_append_bytes(pcb->tx_buf, data, len);
    pcb->retransmit_at = 0;
}

// 受信済みデータをバッファから引数 buf へ読み出し、読み出したバイト数を返す。
// 読み出すデータがない場合は0を返す。
size_t tcp_read(struct tcp_pcb *pcb, void *buf, size_t buf_len) {
    size_t read_len = mbuf_read(&pcb->rx_buf, buf, buf_len);

    // 読み込んだ分だけ受信バッファが余裕ができたので、ウィンドウサイズを更新して通信相手に
    // 続くデータを送信してもらうようにする。
    pcb->local_winsize += read_len;
    return read_len;
}

// PCBに未送信データ・フラグがあれば送信する。
static void tcp_transmit(struct tcp_pcb *pcb) {
    // 再送タイマーが設定されていて、再送時刻にまだ到達していなければ何もしない。
    if (pcb->retransmit_at && sys_uptime() < pcb->retransmit_at) {
        return;
    }

    // コネクションの状態に応じて送信するデータ・フラグを決定する。
    mbuf_t payload = NULL;
    uint8_t flags = 0;
    if (pcb->state == TCP_STATE_ESTABLISHED) {
        // 送信バッファにデータがあれば送信する。
        payload = mbuf_peek(pcb->tx_buf, pcb->remote_winsize);
        if (mbuf_len(payload) > 0) {
            flags |= TCP_ACK | TCP_PSH;
        }

        // 送信するバイト数だけウィンドウサイズを減らすことで、送りすぎないようにする。
        pcb->remote_winsize -= mbuf_len(payload);

        if (pcb->last_seqno == pcb->next_seqno) {
            // 再送処理: 再送回数をインクリメントする。
            pcb->num_retransmits++;
        }
    }

    // 送信待ちフラグをセットする。
    uint32_t pending_flags = pcb->pending_flags;
    if (pending_flags & TCP_PEND_SYN) {
        flags |= TCP_SYN;
    }

    if (pending_flags & TCP_PEND_ACK) {
        flags |= TCP_ACK;
    }

    if (pending_flags & TCP_PEND_FIN) {
        flags |= TCP_FIN;
    }

    // 送信するデータ・フラグがなければパケットを送信しない。
    if (!flags) {
        return;
    }

    // 送信するデータ・フラグがあるので、TCPパケットの構築・送信を行う。
    TRACE("tcp: TX: lport=%d, seq=%08x, ack=%08x, len=%d [ %s%s%s%s%s]",
          pcb->local.port, pcb->next_seqno, pcb->last_ack, mbuf_len(payload),
          (flags & TCP_SYN) ? "SYN " : "", (flags & TCP_FIN) ? "FIN " : "",
          (flags & TCP_ACK) ? "ACK " : "", (flags & TCP_RST) ? "RST " : "",
          (flags & TCP_PSH) ? "PSH " : "");

    // TCPヘッダを構築する。
    struct tcp_header header;
    header.src_port = hton16(pcb->local.port);
    header.dst_port = hton16(pcb->remote.port);
    header.seqno = hton32(pcb->next_seqno);
    header.ackno = (flags & TCP_ACK) ? hton32(pcb->last_ack) : 0;
    header.off_and_ns = 5 << 4;
    header.flags = flags;
    header.win_size = hton16(pcb->local_winsize);
    header.urgent = 0;
    header.checksum = 0;

    // ペイロードのチェックサムを計算する。
    checksum_t checksum;
    checksum_init(&checksum);
    checksum_update_mbuf(&checksum, payload);
    checksum_update(&checksum, &header, sizeof(header));

    // 疑似ヘッダのチェックサムを計算する。
    size_t total_len = sizeof(header) + mbuf_len(payload);
    checksum_update_uint32(&checksum, hton32(pcb->remote.addr));
    checksum_update_uint32(&checksum, hton32(device_get_ipaddr()));
    checksum_update_uint16(&checksum, hton16(total_len));
    checksum_update_uint16(&checksum, hton16(IPV4_PROTO_TCP));

    // チェックサムをヘッダに書き込む。
    header.checksum = checksum_finish(&checksum);

    // パケットを構築する。ペイロードがあれば、ヘッダとペイロードを連結する。
    mbuf_t pkt = mbuf_new(&header, sizeof(header));
    if (payload) {
        mbuf_append(pkt, payload);
    }

    // IPv4の送信処理に回す。
    ipv4_transmit(pcb->remote.addr, IPV4_PROTO_TCP, pkt);

    // 再送タイマーを設定する。
    pcb->retransmit_at =
        sys_uptime()
        + MIN(TCP_TX_MAX_TIMEOUT,
              TCP_TX_INITIAL_TIMEOUT << MIN(pcb->num_retransmits, 8));

    // 最後に送信したシーケンス番号を更新しておき、再送しているのかどうかを判定できるようにする。
    pcb->last_seqno = pcb->next_seqno;
    // 未送信フラグをクリアする。
    pcb->pending_flags = 0;
}

// TCPパケットの受信処理
static void tcp_process(struct tcp_pcb *pcb, ipv4addr_t src_addr,
                        port_t src_port, struct tcp_header *header,
                        mbuf_t payload) {
    // パケットの内容をログに出力する。
    uint32_t seq = ntoh32(header->seqno);
    uint32_t ack = ntoh32(header->ackno);
    uint8_t flags = header->flags;
    TRACE("tcp: RX: lport=%d, seq=%08x, ack=%08x, len=%d [ %s%s%s%s%s]",
          pcb->local.port, seq, ack, mbuf_len(payload),
          (flags & TCP_SYN) ? "SYN " : "", (flags & TCP_FIN) ? "FIN " : "",
          (flags & TCP_ACK) ? "ACK " : "", (flags & TCP_RST) ? "RST " : "",
          (flags & TCP_PSH) ? "PSH " : "");

    if (pcb->state == TCP_STATE_LISTEN) {
        if((flags & TCP_SYN) == 0) {
            // discard packet
            return;
        }

        // create new pcb
        TRACE("tcp: new connection from port %d", src_port);

        struct tcp_pcb *new_pcb = tcp_new(NULL);
        new_pcb->state = TCP_STATE_SYN_RECVED;
        new_pcb->local = pcb->local;
        new_pcb->remote.addr = src_addr;
        new_pcb->remote.port = src_port;
        new_pcb->last_ack = seq + 1;
        new_pcb->pending_flags |= (TCP_PEND_SYN|TCP_PEND_ACK);
        new_pcb->parent = pcb;
        list_push_back(&active_pcbs, &new_pcb->next);
        return;
    }

    // RSTパケットの処理
    if (flags & TCP_RST) {
        WARN("tcp: received RST from %pI4:%d", src_addr, src_port);
        callback_tcp_rst(pcb);
        return;
    }

    // このTCP実装はリオーダリングや SACK (Selective ACK) など面倒な処理をサポートして
    // おらず、TCPパケットが順番に到着することを前提としている。予期したシーケンス番号で
    // なければ同じ確認応答番号のACKパケットを送ることで、次に欲しいデータの再送を促す。
    if (pcb->state != TCP_STATE_SYN_SENT && seq != pcb->last_ack) {
        WARN("tcp: unexpected sequence number: %08x (expected %08x)", seq,
             pcb->last_ack);
        pcb->pending_flags |= TCP_PEND_ACK;
        return;
    }

    // 送信相手のウィンドウサイズを更新する。
    pcb->remote_winsize = ntoh32(header->win_size);

    // コネクションの状態に応じた処理を行う。
    switch (pcb->state) {
        case TCP_STATE_SYN_RECVED: {
            if((flags & TCP_ACK) == 0) {
                WARN("tcp: expected ACK but received %02x", flags);
                break;
            }

            pcb->next_seqno = ack;
            pcb->last_ack = seq;
            pcb->state = TCP_STATE_ESTABLISHED;
            pcb->retransmit_at = 0;
            break;
        }

        // SYNを送信して、SYN+ACKを待っている状態。
        case TCP_STATE_SYN_SENT: {
            // SYN+ACKを受信したかチェック。
            if ((flags & (TCP_SYN | TCP_ACK)) == 0) {
                WARN("tcp: expected SYN+ACK but received %02x", flags);
                break;
            }

            // SYN+ACKを受信したので、ACKを返す。
            pcb->next_seqno = ack;
            pcb->last_ack = seq + 1;
            pcb->state = TCP_STATE_ESTABLISHED;
            pcb->retransmit_at = 0;
            pcb->pending_flags |= TCP_PEND_ACK;
            break;
        }
        // コネクションが確立している状態。データを受信する。
        case TCP_STATE_ESTABLISHED: {
            // 相手がどこまで受信したかをチェック。
            uint32_t acked_len = ack - pcb->next_seqno;
            if (acked_len > 0) {
                // 相手に届いたバイト数分だけ送信バッファから削除する。
                mbuf_discard(&pcb->tx_buf, acked_len);
                pcb->next_seqno += acked_len;
            }

            // 受信したデータを受信バッファにコピーする。
            size_t payload_len = mbuf_len(payload);
            TRACE("tcp: received %d bytes (seq=%x)", payload_len, seq);

            if (0 < payload_len && payload_len <= pcb->local_winsize) {
                // 受信したデータに対するACKを返す。また、ローカルのウィンドウサイズを減らす
                // ことで、相手がデータを送りすぎないようにする。
                pcb->last_ack += mbuf_len(payload);
                pcb->local_winsize -= payload_len;
                pcb->pending_flags |= TCP_PEND_ACK;

                mbuf_append(pcb->rx_buf, payload);
                callback_tcp_data(pcb);
            }

            // FINパケットの処理 (パッシブクローズ)
            if (flags & TCP_FIN) {
                // 簡単のため、全てのデータを送信済みであることを仮定して即座に CLOSED に
                // 遷移する。本来であれば CLOSE_WAIT に遷移して通信を続ける。
                ASSERT(mbuf_len(pcb->tx_buf) == 0);

                // send FIN+ACK
                pcb->state = TCP_STATE_LAST_ACK;
                pcb->retransmit_at = 0;
                pcb->last_ack++;
                pcb->pending_flags |= (TCP_PEND_FIN|TCP_PEND_ACK);
                break;
            }

            break;
        }
        case TCP_STATE_LAST_ACK: {
            if ((flags & TCP_ACK) == 0) {
                WARN("tcp: expected ACK but received %02x", flags);
                break;
            }
            // close
            pcb->state = TCP_STATE_CLOSED;
            callback_tcp_closed(pcb);
            break;
        }
        default:
            WARN("tcp: unexpected packet in state=%d", pcb->state);
            break;
    }
}

// TCPパケットの受信処理。該当するPCBを探してtcp_process()を呼び出す。
void tcp_receive(ipv4addr_t dst, ipv4addr_t src, mbuf_t pkt) {
    // TCPヘッダを読み込む
    struct tcp_header header;
    if (mbuf_read(&pkt, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    // TCPヘッダを pkt から取り除く
    size_t offset = (header.off_and_ns >> 4) * 4;
    if (offset > sizeof(header)) {
        mbuf_discard(&pkt, offset - sizeof(header));
    }

    // 送信元・宛先のIPアドレスとポート番号を取得
    endpoint_t dst_ep, src_ep;
    dst_ep.port = ntoh16(header.dst_port);
    dst_ep.addr = dst;
    src_ep.port = ntoh16(header.src_port);
    src_ep.addr = src;

    struct tcp_pcb *pcb = tcp_lookup(&dst_ep, &src_ep);
    if (!pcb) {
        WARN("tcp: no PCB found for %pI4:%d -> %pI4:%d", src, src_ep.port, dst,
             dst_ep.port);
        return;
    }

    tcp_process(pcb, src, src_ep.port, &header, pkt);
}

// 各PCBをチェックし、未送信データがあれば送信する。
void tcp_flush(void) {
    LIST_FOR_EACH (pcb, &active_pcbs, struct tcp_pcb, next) {
        tcp_transmit(pcb);
    }
}

// TCP実装の初期化
void tcp_init(void) {
    list_init(&active_pcbs);
    for (int i = 0; i < TCP_PCBS_MAX; i++) {
        pcbs[i].in_use = false;
    }
}
