#pragma once
//
//  generate_ipcstub.py で自動生成されたIPCスタブ。
//
//  このファイルを直接編集しないこと。代わりに messages.idl や tools/generate_ipcstub.py を
//  変更するべき。
//
#include <libs/common/types.h>

//
//  メッセージフィールドの定義
//

struct exception_fields {
    task_t task;
    int reason;
};

struct page_fault_fields {
    task_t task;
    uaddr_t uaddr;
    uaddr_t ip;
    unsigned fault;
};
struct page_fault_reply_fields {
};

struct notify_fields {
    notifications_t notifications;
};

struct notify_irq_fields {
};

struct notify_timer_fields {
};

struct async_recv_fields {
};
struct async_recv_reply_fields {
};

struct ping_fields {
    int value;
};
struct ping_reply_fields {
    int value;
};

struct spawn_task_fields {
    char name[32];
};
struct spawn_task_reply_fields {
    task_t task;
};

struct destroy_task_fields {
    task_t task;
};
struct destroy_task_reply_fields {
};

struct service_lookup_fields {
    char name[64];
};
struct service_lookup_reply_fields {
    task_t task;
};

struct service_register_fields {
    char name[64];
};
struct service_register_reply_fields {
};

struct watch_tasks_fields {
};
struct watch_tasks_reply_fields {
};

struct task_destroyed_fields {
    task_t task;
};

struct vm_map_physical_fields {
    paddr_t paddr;
    size_t size;
    int map_flags;
};
struct vm_map_physical_reply_fields {
    uaddr_t uaddr;
};

struct vm_alloc_physical_fields {
    size_t size;
    int alloc_flags;
    int map_flags;
};
struct vm_alloc_physical_reply_fields {
    uaddr_t uaddr;
    paddr_t paddr;
};

struct blk_read_fields {
    unsigned sector;
    size_t offset;
    size_t len;
};
struct blk_read_reply_fields {
    uint8_t data[1024];
    size_t data_len;
};

struct blk_write_fields {
    unsigned sector;
    size_t offset;
    uint8_t data[1024];
    size_t data_len;
};
struct blk_write_reply_fields {
};

struct net_open_fields {
};
struct net_open_reply_fields {
    uint8_t macaddr[6];
};

struct net_recv_fields {
    uint8_t payload[1500];
    size_t payload_len;
};

struct net_send_fields {
    uint8_t payload[1500];
    size_t payload_len;
};
struct net_send_reply_fields {
};

struct fs_open_fields {
    char path[256];
    int flags;
};
struct fs_open_reply_fields {
    int fd;
};

struct fs_close_fields {
    int fd;
};
struct fs_close_reply_fields {
};

struct fs_read_fields {
    int fd;
    size_t len;
};
struct fs_read_reply_fields {
    uint8_t data[1024];
    size_t data_len;
};

struct fs_write_fields {
    int fd;
    uint8_t data[1024];
    size_t data_len;
};
struct fs_write_reply_fields {
    size_t written_len;
};

struct fs_readdir_fields {
    char path[256];
    int index;
};
struct fs_readdir_reply_fields {
    char name[256];
    int type;
    size_t filesize;
};

struct fs_mkfile_fields {
    char path[256];
};
struct fs_mkfile_reply_fields {
};

struct fs_mkdir_fields {
    char path[256];
};
struct fs_mkdir_reply_fields {
};

struct fs_delete_fields {
    char path[256];
};
struct fs_delete_reply_fields {
};

struct tcpip_connect_fields {
    uint32_t dst_addr;
    uint16_t dst_port;
};
struct tcpip_connect_reply_fields {
    int sock;
};

struct tcpip_listen_fields {
    uint16_t listen_port;
};
struct tcpip_listen_reply_fields {
    int sock;
};

struct tcpip_close_fields {
    int sock;
};
struct tcpip_close_reply_fields {
};

struct tcpip_write_fields {
    int sock;
    uint8_t data[1024];
    size_t data_len;
};
struct tcpip_write_reply_fields {
};

struct tcpip_read_fields {
    int sock;
};
struct tcpip_read_reply_fields {
    uint8_t data[1024];
    size_t data_len;
};

struct tcpip_dns_resolve_fields {
    char hostname[256];
};
struct tcpip_dns_resolve_reply_fields {
    uint32_t addr;
};

struct tcpip_data_fields {
    int sock;
};

struct tcpip_closed_fields {
    int sock;
};



#define EXCEPTION_MSG 1
#define PAGE_FAULT_MSG 2
#define PAGE_FAULT_REPLY_MSG 3
#define NOTIFY_MSG 4
#define NOTIFY_IRQ_MSG 5
#define NOTIFY_TIMER_MSG 6
#define ASYNC_RECV_MSG 7
#define ASYNC_RECV_REPLY_MSG 8
#define PING_MSG 9
#define PING_REPLY_MSG 10
#define SPAWN_TASK_MSG 11
#define SPAWN_TASK_REPLY_MSG 12
#define DESTROY_TASK_MSG 13
#define DESTROY_TASK_REPLY_MSG 14
#define SERVICE_LOOKUP_MSG 15
#define SERVICE_LOOKUP_REPLY_MSG 16
#define SERVICE_REGISTER_MSG 17
#define SERVICE_REGISTER_REPLY_MSG 18
#define WATCH_TASKS_MSG 19
#define WATCH_TASKS_REPLY_MSG 20
#define TASK_DESTROYED_MSG 21
#define VM_MAP_PHYSICAL_MSG 22
#define VM_MAP_PHYSICAL_REPLY_MSG 23
#define VM_ALLOC_PHYSICAL_MSG 24
#define VM_ALLOC_PHYSICAL_REPLY_MSG 25
#define BLK_READ_MSG 26
#define BLK_READ_REPLY_MSG 27
#define BLK_WRITE_MSG 28
#define BLK_WRITE_REPLY_MSG 29
#define NET_OPEN_MSG 30
#define NET_OPEN_REPLY_MSG 31
#define NET_RECV_MSG 32
#define NET_SEND_MSG 33
#define NET_SEND_REPLY_MSG 34
#define FS_OPEN_MSG 35
#define FS_OPEN_REPLY_MSG 36
#define FS_CLOSE_MSG 37
#define FS_CLOSE_REPLY_MSG 38
#define FS_READ_MSG 39
#define FS_READ_REPLY_MSG 40
#define FS_WRITE_MSG 41
#define FS_WRITE_REPLY_MSG 42
#define FS_READDIR_MSG 43
#define FS_READDIR_REPLY_MSG 44
#define FS_MKFILE_MSG 45
#define FS_MKFILE_REPLY_MSG 46
#define FS_MKDIR_MSG 47
#define FS_MKDIR_REPLY_MSG 48
#define FS_DELETE_MSG 49
#define FS_DELETE_REPLY_MSG 50
#define TCPIP_CONNECT_MSG 51
#define TCPIP_CONNECT_REPLY_MSG 52
#define TCPIP_LISTEN_MSG 53
#define TCPIP_LISTEN_REPLY_MSG 54
#define TCPIP_CLOSE_MSG 55
#define TCPIP_CLOSE_REPLY_MSG 56
#define TCPIP_WRITE_MSG 57
#define TCPIP_WRITE_REPLY_MSG 58
#define TCPIP_READ_MSG 59
#define TCPIP_READ_REPLY_MSG 60
#define TCPIP_DNS_RESOLVE_MSG 61
#define TCPIP_DNS_RESOLVE_REPLY_MSG 62
#define TCPIP_DATA_MSG 63
#define TCPIP_CLOSED_MSG 64

//
//  各種マクロの定義
//
#define IPCSTUB_MESSAGE_FIELDS \
    struct exception_fields exception; \
    struct page_fault_fields page_fault; \
    struct page_fault_reply_fields page_fault_reply; \
    struct notify_fields notify; \
    struct notify_irq_fields notify_irq; \
    struct notify_timer_fields notify_timer; \
    struct async_recv_fields async_recv; \
    struct async_recv_reply_fields async_recv_reply; \
    struct ping_fields ping; \
    struct ping_reply_fields ping_reply; \
    struct spawn_task_fields spawn_task; \
    struct spawn_task_reply_fields spawn_task_reply; \
    struct destroy_task_fields destroy_task; \
    struct destroy_task_reply_fields destroy_task_reply; \
    struct service_lookup_fields service_lookup; \
    struct service_lookup_reply_fields service_lookup_reply; \
    struct service_register_fields service_register; \
    struct service_register_reply_fields service_register_reply; \
    struct watch_tasks_fields watch_tasks; \
    struct watch_tasks_reply_fields watch_tasks_reply; \
    struct task_destroyed_fields task_destroyed; \
    struct vm_map_physical_fields vm_map_physical; \
    struct vm_map_physical_reply_fields vm_map_physical_reply; \
    struct vm_alloc_physical_fields vm_alloc_physical; \
    struct vm_alloc_physical_reply_fields vm_alloc_physical_reply; \
    struct blk_read_fields blk_read; \
    struct blk_read_reply_fields blk_read_reply; \
    struct blk_write_fields blk_write; \
    struct blk_write_reply_fields blk_write_reply; \
    struct net_open_fields net_open; \
    struct net_open_reply_fields net_open_reply; \
    struct net_recv_fields net_recv; \
    struct net_send_fields net_send; \
    struct net_send_reply_fields net_send_reply; \
    struct fs_open_fields fs_open; \
    struct fs_open_reply_fields fs_open_reply; \
    struct fs_close_fields fs_close; \
    struct fs_close_reply_fields fs_close_reply; \
    struct fs_read_fields fs_read; \
    struct fs_read_reply_fields fs_read_reply; \
    struct fs_write_fields fs_write; \
    struct fs_write_reply_fields fs_write_reply; \
    struct fs_readdir_fields fs_readdir; \
    struct fs_readdir_reply_fields fs_readdir_reply; \
    struct fs_mkfile_fields fs_mkfile; \
    struct fs_mkfile_reply_fields fs_mkfile_reply; \
    struct fs_mkdir_fields fs_mkdir; \
    struct fs_mkdir_reply_fields fs_mkdir_reply; \
    struct fs_delete_fields fs_delete; \
    struct fs_delete_reply_fields fs_delete_reply; \
    struct tcpip_connect_fields tcpip_connect; \
    struct tcpip_connect_reply_fields tcpip_connect_reply; \
    struct tcpip_listen_fields tcpip_listen; \
    struct tcpip_listen_reply_fields tcpip_listen_reply; \
    struct tcpip_close_fields tcpip_close; \
    struct tcpip_close_reply_fields tcpip_close_reply; \
    struct tcpip_write_fields tcpip_write; \
    struct tcpip_write_reply_fields tcpip_write_reply; \
    struct tcpip_read_fields tcpip_read; \
    struct tcpip_read_reply_fields tcpip_read_reply; \
    struct tcpip_dns_resolve_fields tcpip_dns_resolve; \
    struct tcpip_dns_resolve_reply_fields tcpip_dns_resolve_reply; \
    struct tcpip_data_fields tcpip_data; \
    struct tcpip_closed_fields tcpip_closed; \

#define IPCSTUB_MSGID_MAX 64
#define IPCSTUB_MSGID2STR \
    (const char *[]){ \
     \
        [1] = "exception", \
     \
        [2] = "page_fault", \
        [3] = "page_fault_reply", \
     \
        [4] = "notify", \
     \
        [5] = "notify_irq", \
     \
        [6] = "notify_timer", \
     \
        [7] = "async_recv", \
        [8] = "async_recv_reply", \
     \
        [9] = "ping", \
        [10] = "ping_reply", \
     \
        [11] = "spawn_task", \
        [12] = "spawn_task_reply", \
     \
        [13] = "destroy_task", \
        [14] = "destroy_task_reply", \
     \
        [15] = "service_lookup", \
        [16] = "service_lookup_reply", \
     \
        [17] = "service_register", \
        [18] = "service_register_reply", \
     \
        [19] = "watch_tasks", \
        [20] = "watch_tasks_reply", \
     \
        [21] = "task_destroyed", \
     \
        [22] = "vm_map_physical", \
        [23] = "vm_map_physical_reply", \
     \
        [24] = "vm_alloc_physical", \
        [25] = "vm_alloc_physical_reply", \
     \
        [26] = "blk_read", \
        [27] = "blk_read_reply", \
     \
        [28] = "blk_write", \
        [29] = "blk_write_reply", \
     \
        [30] = "net_open", \
        [31] = "net_open_reply", \
     \
        [32] = "net_recv", \
     \
        [33] = "net_send", \
        [34] = "net_send_reply", \
     \
        [35] = "fs_open", \
        [36] = "fs_open_reply", \
     \
        [37] = "fs_close", \
        [38] = "fs_close_reply", \
     \
        [39] = "fs_read", \
        [40] = "fs_read_reply", \
     \
        [41] = "fs_write", \
        [42] = "fs_write_reply", \
     \
        [43] = "fs_readdir", \
        [44] = "fs_readdir_reply", \
     \
        [45] = "fs_mkfile", \
        [46] = "fs_mkfile_reply", \
     \
        [47] = "fs_mkdir", \
        [48] = "fs_mkdir_reply", \
     \
        [49] = "fs_delete", \
        [50] = "fs_delete_reply", \
     \
        [51] = "tcpip_connect", \
        [52] = "tcpip_connect_reply", \
     \
        [53] = "tcpip_listen", \
        [54] = "tcpip_listen_reply", \
     \
        [55] = "tcpip_close", \
        [56] = "tcpip_close_reply", \
     \
        [57] = "tcpip_write", \
        [58] = "tcpip_write_reply", \
     \
        [59] = "tcpip_read", \
        [60] = "tcpip_read_reply", \
     \
        [61] = "tcpip_dns_resolve", \
        [62] = "tcpip_dns_resolve_reply", \
     \
        [63] = "tcpip_data", \
     \
        [64] = "tcpip_closed", \
     \
    }

#define IPCSTUB_STATIC_ASSERTIONS \
    _Static_assert( \
        sizeof(struct exception_fields) < 4096, \
        "'exception' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct page_fault_fields) < 4096, \
        "'page_fault' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct page_fault_reply_fields) < 4096, \
        "'page_fault_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct notify_fields) < 4096, \
        "'notify' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct notify_irq_fields) < 4096, \
        "'notify_irq' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct notify_timer_fields) < 4096, \
        "'notify_timer' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct async_recv_fields) < 4096, \
        "'async_recv' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct async_recv_reply_fields) < 4096, \
        "'async_recv_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct ping_fields) < 4096, \
        "'ping' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct ping_reply_fields) < 4096, \
        "'ping_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct spawn_task_fields) < 4096, \
        "'spawn_task' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct spawn_task_reply_fields) < 4096, \
        "'spawn_task_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct destroy_task_fields) < 4096, \
        "'destroy_task' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct destroy_task_reply_fields) < 4096, \
        "'destroy_task_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct service_lookup_fields) < 4096, \
        "'service_lookup' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct service_lookup_reply_fields) < 4096, \
        "'service_lookup_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct service_register_fields) < 4096, \
        "'service_register' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct service_register_reply_fields) < 4096, \
        "'service_register_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct watch_tasks_fields) < 4096, \
        "'watch_tasks' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct watch_tasks_reply_fields) < 4096, \
        "'watch_tasks_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct task_destroyed_fields) < 4096, \
        "'task_destroyed' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct vm_map_physical_fields) < 4096, \
        "'vm_map_physical' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct vm_map_physical_reply_fields) < 4096, \
        "'vm_map_physical_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct vm_alloc_physical_fields) < 4096, \
        "'vm_alloc_physical' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct vm_alloc_physical_reply_fields) < 4096, \
        "'vm_alloc_physical_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct blk_read_fields) < 4096, \
        "'blk_read' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct blk_read_reply_fields) < 4096, \
        "'blk_read_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct blk_write_fields) < 4096, \
        "'blk_write' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct blk_write_reply_fields) < 4096, \
        "'blk_write_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct net_open_fields) < 4096, \
        "'net_open' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct net_open_reply_fields) < 4096, \
        "'net_open_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct net_recv_fields) < 4096, \
        "'net_recv' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct net_send_fields) < 4096, \
        "'net_send' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct net_send_reply_fields) < 4096, \
        "'net_send_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_open_fields) < 4096, \
        "'fs_open' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_open_reply_fields) < 4096, \
        "'fs_open_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_close_fields) < 4096, \
        "'fs_close' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_close_reply_fields) < 4096, \
        "'fs_close_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_read_fields) < 4096, \
        "'fs_read' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_read_reply_fields) < 4096, \
        "'fs_read_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_write_fields) < 4096, \
        "'fs_write' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_write_reply_fields) < 4096, \
        "'fs_write_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_readdir_fields) < 4096, \
        "'fs_readdir' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_readdir_reply_fields) < 4096, \
        "'fs_readdir_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_mkfile_fields) < 4096, \
        "'fs_mkfile' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_mkfile_reply_fields) < 4096, \
        "'fs_mkfile_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_mkdir_fields) < 4096, \
        "'fs_mkdir' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_mkdir_reply_fields) < 4096, \
        "'fs_mkdir_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_delete_fields) < 4096, \
        "'fs_delete' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct fs_delete_reply_fields) < 4096, \
        "'fs_delete_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_connect_fields) < 4096, \
        "'tcpip_connect' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_connect_reply_fields) < 4096, \
        "'tcpip_connect_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_listen_fields) < 4096, \
        "'tcpip_listen' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_listen_reply_fields) < 4096, \
        "'tcpip_listen_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_close_fields) < 4096, \
        "'tcpip_close' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_close_reply_fields) < 4096, \
        "'tcpip_close_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_write_fields) < 4096, \
        "'tcpip_write' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_write_reply_fields) < 4096, \
        "'tcpip_write_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_read_fields) < 4096, \
        "'tcpip_read' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_read_reply_fields) < 4096, \
        "'tcpip_read_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_dns_resolve_fields) < 4096, \
        "'tcpip_dns_resolve' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_dns_resolve_reply_fields) < 4096, \
        "'tcpip_dns_resolve_reply' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_data_fields) < 4096, \
        "'tcpip_data' message is too large, should be less than 4096 bytes" \
    ); \
    _Static_assert( \
        sizeof(struct tcpip_closed_fields) < 4096, \
        "'tcpip_closed' message is too large, should be less than 4096 bytes" \
    ); \

