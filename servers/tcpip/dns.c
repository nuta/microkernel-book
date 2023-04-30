//
// DNSクライアント
//
#include "dns.h"
#include "main.h"
#include "udp.h"
#include <libs/common/endian.h>
#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/malloc.h>

// DNSクライアントに利用するUDPソケット
static udp_sock_t udp_sock;
// DNSキャッシュサーバのIPアドレス
static ipv4addr_t dns_server_ipaddr;
// 応答待ちのDNSクエリのリスト
static list_t dns_requests = LIST_INIT(dns_requests);
// 次に使うDNSクエリのID
static uint16_t next_query_id = 1;

// DNS解決を開始する。
error_t dns_query(const char *hostname, void *arg) {
    // DNSヘッダを構築する
    struct dns_header header;
    header.id = hton16(next_query_id);
    header.flags = hton16(DNS_FLAG_RD);  // 通常のクエリ、再帰的な解決を要求
    header.num_queries = hton16(1);      // クエリは1つ
    header.num_answers = 0;
    header.num_authority = 0;
    header.num_additional = 0;
    mbuf_t m = mbuf_new(&header, sizeof(header));

    // QUESTIONセクション: QNAMEを書き込む
    char *s_orig = strdup(hostname);
    char *s = s_orig;
    while (*s != '\0') {
        // '.'か'\0'までの文字列をラベルとして取り出す
        char *label = s;
        while (*s != '.' && *s != '\0') {
            s++;
        }

        // '.'を'\0'に置き換えて文字列を終端する
        if (*s != '\0') {
            *s = '\0';
            s++;
        }

        // ラベルが長すぎないかチェックする
        size_t label_len = strlen(label);
        if (label_len > 63) {
            WARN("dns: too long hostname: %s", hostname);
            free(s);
            return ERR_INVALID_ARG;
        }

        // ラベルを書き込む
        mbuf_append_bytes(m, &(uint8_t){label_len}, 1);  // ラベルの長さ
        mbuf_append_bytes(m, label, strlen(label));      // ラベルの文字列
    }

    // QNAMEの終端
    mbuf_append_bytes(m, &(uint8_t){0}, 1);

    // QUESTIONセクション: QTYPEとQCLASSを書き込む
    uint16_t qtype_ne = hton16(DNS_QTYPE_A);
    uint16_t qclass_ne = hton16(DNS_QCLASS_IN);
    mbuf_append_bytes(m, (uint8_t *) &qtype_ne, sizeof(uint16_t));
    mbuf_append_bytes(m, (uint8_t *) &qclass_ne, sizeof(uint16_t));

    // DNSクエリをキューに追加する
    udp_sendto_mbuf(udp_sock, dns_server_ipaddr, 53, m);
    // UDPパケットを送信する
    udp_transmit(udp_sock);

    struct dns_request *req = malloc(sizeof(*req));
    req->id = next_query_id;
    req->arg = arg;
    list_elem_init(&req->next);
    list_push_back(&dns_requests, &req->next);
    next_query_id++;

    free(s_orig);
    return OK;
}

// DNSパケットのラベル (QNAME/NAME) を読み飛ばす
static void skip_labels(mbuf_t *payload) {
    uint8_t len;
    do {
        // ラベルの長さを読み込む
        if (mbuf_read(payload, &len, sizeof(len)) != sizeof(len)) {
            return;
        }

        // 圧縮されたラベルであれば1バイト読み飛ばすだけでよい
        if (len & 0xc0) {
            mbuf_discard(payload, 1);
            break;
        }

        // 通常のラベルであれば、その長さ分だけ読み飛ばす
        mbuf_discard(payload, len);
    } while (len > 0);
}

// DNSパケットを処理する
static void dns_process(mbuf_t payload) {
    // DNSヘッダを読み込む
    struct dns_header header;
    if (mbuf_read(&payload, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    // QUESTIONセクションを読み飛ばす
    uint16_t num_queries = ntoh16(header.num_queries);
    for (uint16_t i = 0; i < num_queries; i++) {
        skip_labels(&payload);
        struct dns_query_footer footer;
        if (mbuf_read(&payload, &footer, sizeof(footer)) != sizeof(footer)) {
            return;
        }
    }

    // ANSWERセクション
    uint16_t num_answers = ntoh16(header.num_answers);
    for (uint16_t i = 0; i < num_answers; i++) {
        skip_labels(&payload);

        // NAMEより後の部分を読み込む
        struct dns_answer_footer footer;
        if (mbuf_read(&payload, &footer, sizeof(footer)) != sizeof(footer)) {
            return;
        }

        // Aレコードでなければ読み飛ばす
        uint16_t data_len = ntoh16(footer.len);
        if (ntoh16(footer.type) != DNS_QTYPE_A) {
            mbuf_discard(&payload, data_len);
            continue;
        }

        // Aレコードのデータを読み込む
        uint32_t data;
        if (mbuf_read(&payload, &data, sizeof(data)) != sizeof(data)) {
            return;
        }

        // 解決したIPアドレスをコールバック関数に渡す
        uint32_t id = ntoh16(header.id);
        ipv4addr_t addr = ntoh32(data);
        LIST_FOR_EACH (req, &dns_requests, struct dns_request, next) {
            if (id == req->id) {
                callback_dns_got_answer(addr, req->arg);
                list_remove(&req->next);
                free(req);
            }
        }
    }
}

// 受信済みのDNSパケットを処理する
void dns_receive(void) {
    while (true) {
        // 受信済みのパケットを取り出す
        ipv4addr_t src;
        port_t src_port;
        mbuf_t payload = udp_recv_mbuf(udp_sock, &src, &src_port);
        if (!payload) {
            break;
        }

        if (src != dns_server_ipaddr) {
            WARN("received a DNS answer from an unknown address %pI4", src);
            continue;
        }

        dns_process(payload);
    }
}

// DNSキャッシュサーバのIPアドレスを設定する
void dns_set_name_server(ipv4addr_t ipaddr) {
    dns_server_ipaddr = ipaddr;
}

// DNSクライアントの初期化
void dns_init(void) {
    udp_sock = udp_new();
    udp_bind(udp_sock, IPV4_ADDR_UNSPECIFIED, 3535);
}
