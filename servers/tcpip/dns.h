#pragma once
#include "ipv4.h"
#include <libs/common/list.h>
#include <libs/common/types.h>

// DNSクエリの種類: Aレコード
#define DNS_QTYPE_A 0x0001
// DNSクラス: インターネット
#define DNS_QCLASS_IN 0x0001

// DNSヘッダのフラグ
#define DNS_FLAG_RD (1 << 7)  // 再帰的な解決を要求

// 応答待ちDNSクエリの情報
struct dns_request {
    list_elem_t next;  // リスト中の次の要素
    uint16_t id;       // トランザクションID
    void *arg;         // 任意の引数
};

// DNSヘッダ
struct dns_header {
    uint16_t id;              // トランザクションID
    uint16_t flags;           // 各フラグ
    uint16_t num_queries;     // QUESTIONセクションのクエリ数
    uint16_t num_answers;     // ANSWERセクションのレコード数
    uint16_t num_authority;   // AUTHORITYセクションのレコード数
    uint16_t num_additional;  // ADDITIONALセクションのレコード数
} __packed;

// QUESTIONセクションのクエリのうち、QNAMEより後ろの部分
struct dns_query_footer {
    uint16_t qtype;   // 種類
    uint16_t qclass;  // クラス
} __packed;

// ANSWERセクションのレコードのうち、NAMEより後ろの部分
struct dns_answer_footer {
    uint16_t type;   // 種類
    uint16_t class;  // クラス
    uint32_t ttl;    // TTL
    uint16_t len;    // データ長
    uint8_t data[];  // データ
} __packed;

error_t dns_query(const char *hostname, void *arg);
void dns_receive(void);
void dns_set_name_server(ipv4addr_t ipaddr);
void dns_init(void);
