//
// DHCPクライアント
//
#include "dhcp.h"
#include "device.h"
#include "dns.h"
#include "udp.h"
#include <libs/common/endian.h>
#include <libs/common/list.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/malloc.h>

// DHCPクライアントで利用するUDPソケット
static udp_sock_t udp_sock;

// DHCPパケットの送信
void dhcp_transmit(enum dhcp_type type, ipv4addr_t requested_addr) {
    // ヘッダを構築する
    struct dhcp_header header;
    memset(&header, 0, sizeof(header));
    header.op = BOOTP_OP_REQUEST;
    header.hw_type = BOOTP_HWTYPE_ETHERNET;
    header.hw_len = MACADDR_LEN;
    header.hops = 0;
    header.xid = hton32(0x1234abcd);
    header.secs = 0;
    header.flags = hton16(0x8000);
    header.client_ipaddr = 0;
    header.your_ipaddr = 0;
    header.server_ipaddr = 0;
    header.gateway_ipaddr = 0;
    memcpy(header.client_hwaddr, device_get_macaddr(), MACADDR_LEN);
    header.magic = hton32(DHCP_MAGIC);

    mbuf_t m = mbuf_new(&header, sizeof(header));

    // オプション: DHCPパケットの種類
    struct dhcp_type_option type_opt;
    type_opt.dhcp_type = type;
    mbuf_append_bytes(m, &(uint8_t[2]){DHCP_OPTION_DHCP_TYPE, sizeof(type_opt)},
                      2);
    mbuf_append_bytes(m, &type_opt, sizeof(type_opt));

    // オプション: DHCP REQUESTの場合のときに指定する、要求するIPアドレス
    if (requested_addr != IPV4_ADDR_UNSPECIFIED) {
        struct dhcp_reqaddr_option addr_opt;
        addr_opt.ipaddr = hton32(requested_addr);
        mbuf_append_bytes(
            m, &(uint8_t[2]){DHCP_OPTION_REQUESTED_ADDR, sizeof(addr_opt)}, 2);
        mbuf_append_bytes(m, &addr_opt, sizeof(addr_opt));
    }

    // オプション: DHCPサーバに要求する追加情報
    struct dhcp_params_option params_opt;
    params_opt.params[0] = DHCP_OPTION_NETMASK;  // ネットマスク
    params_opt.params[1] = DHCP_OPTION_ROUTER;   // デフォルトゲートウェイ
    params_opt.params[2] = DHCP_OPTION_DNS;      // デフォルトゲートウェイ
    mbuf_append_bytes(
        m, &(uint8_t[2]){DHCP_OPTION_PARAM_LIST, sizeof(params_opt)}, 2);
    mbuf_append_bytes(m, &params_opt, sizeof(params_opt));

    // オプション: 終了マーカー
    mbuf_append_bytes(m, &(uint8_t){DHCP_OPTION_END}, 1);

    // 4バイトにアラインするようにパディングを追加する
    size_t padding_len = 4 - (mbuf_len(m) % 4) + 8;
    while (padding_len-- > 0) {
        mbuf_append_bytes(m, &(uint8_t){DHCP_OPTION_END}, 1);
    }

    // DHCPサーバ (ブロードキャストアドレス) に送信する。「67」はDHCPサーバのポート。
    udp_sendto_mbuf(udp_sock, IPV4_ADDR_BROADCAST, 67, m);
    // キューに入れたパケットを送信する
    udp_transmit(udp_sock);
}

// DHCPパケットの受信
static void dhcp_process(mbuf_t payload) {
    // DHCPパケットのヘッダを読み込む
    struct dhcp_header header;
    if (mbuf_read(&payload, &header, sizeof(header)) != sizeof(header)) {
        return;
    }

    // 提案されたIPアドレス
    ipv4addr_t your_ipaddr = ntoh32(header.your_ipaddr);

    // オプションを読み込む
    enum dhcp_type type = 0;
    ipv4addr_t gateway = IPV4_ADDR_UNSPECIFIED;
    ipv4addr_t netmask = IPV4_ADDR_UNSPECIFIED;
    ipv4addr_t dns_server = IPV4_ADDR_UNSPECIFIED;
    while (!mbuf_is_empty(payload)) {
        // オプションの種類を読み込む
        uint8_t option_type;
        if (mbuf_read(&payload, &option_type, 1) != 1) {
            break;
        }

        // 終わりに達したら終了
        if (option_type == DHCP_OPTION_END) {
            break;
        }

        // オプションの長さを読み込む
        uint8_t option_len;
        if (mbuf_read(&payload, &option_len, 1) != 1) {
            break;
        }

// オプションの長さが正しいかチェックする
#define CHECK_OPTION_LEN(option_type, option_len, opt_struct)                  \
    if (option_len != sizeof(opt_struct)) {                                    \
        WARN("invalid dhcp option len (%d) for option type %d", option_len,    \
             option_type);                                                     \
        return;                                                                \
    }

        // オプションの内容を読み込む
        switch (option_type) {
            // DHCPパケットの種類
            case DHCP_OPTION_DHCP_TYPE: {
                struct dhcp_type_option opt;
                CHECK_OPTION_LEN(option_type, option_len, opt);
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                type = opt.dhcp_type;
                break;
            }
            // ネットマスク
            case DHCP_OPTION_NETMASK: {
                struct dhcp_netmask_option opt;
                CHECK_OPTION_LEN(option_type, option_len, opt);
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                netmask = ntoh32(opt.netmask);
                break;
            }
            // デフォルトゲートウェイ
            case DHCP_OPTION_ROUTER: {
                struct dhcp_router_option opt;
                CHECK_OPTION_LEN(option_type, option_len, opt);
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                gateway = ntoh32(opt.router);
                break;
            }
            // DNSキャッシュサーバ
            case DHCP_OPTION_DNS: {
                struct dhcp_dns_option opt;
                CHECK_OPTION_LEN(option_type, option_len, opt);
                if (mbuf_read(&payload, &opt, sizeof(opt)) != sizeof(opt)) {
                    break;
                }

                dns_server = ntoh32(opt.dns);
                break;
            }
            default:
                mbuf_discard(&payload, option_len);
        }
    }

    // パケットの種類に応じた処理を行う
    switch (type) {
        // DHCP OFFER
        case DHCP_TYPE_OFFER:
            // 提案されたIPアドレスを割り当てるようにリクエストを送信する
            dhcp_transmit(DHCP_TYPE_REQUEST, your_ipaddr);
            break;
        // DHCP ACK
        case DHCP_TYPE_ACK:
            // 必要な情報が与えられたかチェックする
            if (netmask == IPV4_ADDR_UNSPECIFIED
                || gateway == IPV4_ADDR_UNSPECIFIED
                || dns_server == IPV4_ADDR_UNSPECIFIED) {
                WARN(
                    "netmask, router, or dns server DHCP option is not included, "
                    "discarding a DHCP ACK..");
                break;
            }

            // IPアドレスが割り当てられたので、設定を更新してDHCPによる自動設定を終了する
            INFO("dhcp: leased ip=%pI4, netmask=%pI4, gateway=%pI4",
                 your_ipaddr, netmask, gateway);
            device_set_ip_addrs(your_ipaddr, netmask, gateway);
            dns_set_name_server(dns_server);
            break;
        default:
            WARN("ignoring a DHCP message (dhcp_type=%d)", type);
    }
}

// 受信済みのDHCPパケットを処理する
void dhcp_receive(void) {
    while (true) {
        ipv4addr_t src;
        port_t src_port;
        mbuf_t payload = udp_recv_mbuf(udp_sock, &src, &src_port);
        if (!payload) {
            break;
        }

        dhcp_process(payload);
    }
}

// DHCPクライアントの初期化
void dhcp_init(void) {
    udp_sock = udp_new();
    udp_bind(udp_sock, IPV4_ADDR_UNSPECIFIED, 68);
}
