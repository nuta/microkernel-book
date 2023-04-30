#include "bootfs.h"
#include "page_fault.h"
#include "pm.h"
#include "task.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>
#include <libs/user/syscall.h>
#include <libs/user/task.h>

// BootFSにあるサーバのうちBOOT_SERVERSで指定されているものを自動起動する。
static void spawn_servers(void) {
    int num_launched = 0;
    struct bootfs_file *file;
    for (int i = 0; (file = bootfs_open_iter(i)) != NULL; i++) {
        // 自動起動するサーバ名のリスト。空白で区切られている。
        char *startups = BOOT_SERVERS;

        // BOOT_STARTSの各サーバ名と比較する。
        while (*startups != '\0') {
            // ファイル名とサーバ名が一致すれば起動する。
            size_t len = strlen(file->name);
            if (!strncmp(file->name, startups, len)
                && (startups[len] == '\0' || startups[len] == ' ')) {
                ASSERT_OK(task_spawn(file));
                num_launched++;
                break;
            }

            // 一致しなかった。次のサーバ名へ進める。
            while (*startups != '\0' && *startups != ' ') {
                startups++;
            }

            // スペースはスキップする。
            while (*startups == ' ') {
                startups++;
            }
        }
    }

    if (!num_launched) {
        WARN("no servers to launch");
    }
}

void main(void) {
    bootfs_init();
    spawn_servers();

    // service_dump() を後で呼び出すためのタイマーを設定する。
    // 5秒あれば全てのサーバが起動するはず。
    sys_time(5000);

    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case PING_MSG: {
                int value = m.ping.value;
                m.type = PING_REPLY_MSG;
                m.ping_reply.value = value;
                ipc_reply(m.src, &m);
                break;
            }
            case NOTIFY_TIMER_MSG: {
                service_dump();
                break;
            }
            case WATCH_TASKS_MSG: {
                struct task *task = task_find(m.src);
                ASSERT(task);

                task->watch_tasks = true;

                m.type = WATCH_TASKS_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            case SERVICE_LOOKUP_MSG: {
                struct task *task = task_find(m.src);
                ASSERT(task);

                char name[sizeof(m.service_lookup.name)];
                strcpy_safe(name, sizeof(name), m.service_lookup.name);

                task_t server_task = service_lookup_or_wait(task, name);
                if (server_task == ERR_WOULD_BLOCK) {
                    continue;
                }

                m.type = SERVICE_LOOKUP_REPLY_MSG;
                m.service_lookup_reply.task = server_task;
                ipc_reply(m.src, &m);
                break;
            }
            case SERVICE_REGISTER_MSG: {
                struct task *task = task_find(m.src);
                ASSERT(task);

                char name[sizeof(m.service_register.name)];
                strcpy_safe(name, sizeof(name), m.service_register.name);

                service_register(task, name);

                m.type = SERVICE_REGISTER_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            case SPAWN_TASK_MSG: {
                char name[sizeof(m.spawn_task.name)];
                strcpy_safe(name, sizeof(name), m.spawn_task.name);

                struct bootfs_file *file = bootfs_open(name);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                task_t task_or_err = task_spawn(file);
                if (IS_ERROR(task_or_err)) {
                    ipc_reply_err(m.src, task_or_err);
                    break;
                }

                m.type = SPAWN_TASK_REPLY_MSG;
                m.spawn_task_reply.task = task_or_err;
                ipc_reply(m.src, &m);
                break;
            }
            case DESTROY_TASK_MSG: {
                task_destroy_by_tid(m.destroy_task.task);
                m.type = DESTROY_TASK_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            case VM_MAP_PHYSICAL_MSG: {
                struct task *task = task_find(m.src);
                ASSERT(task);

                uaddr_t uaddr;
                map_pages(task, m.vm_map_physical.size,
                          m.vm_map_physical.map_flags, m.vm_map_physical.paddr,
                          &uaddr);

                m.type = VM_MAP_PHYSICAL_MSG;
                m.vm_map_physical_reply.uaddr = uaddr;
                ipc_reply(m.src, &m);
                break;
            }
            case VM_ALLOC_PHYSICAL_MSG: {
                struct task *task = task_find(m.src);
                ASSERT(task);

                paddr_t paddr;
                uaddr_t uaddr;
                alloc_pages(task, m.vm_alloc_physical.size,
                            m.vm_alloc_physical.alloc_flags,
                            m.vm_alloc_physical.map_flags, &paddr, &uaddr);

                m.type = VM_ALLOC_PHYSICAL_REPLY_MSG;
                m.vm_alloc_physical_reply.uaddr = uaddr;
                m.vm_alloc_physical_reply.paddr = paddr;
                ipc_reply(m.src, &m);
                break;
            }
            case EXCEPTION_MSG: {
                if (m.src != FROM_KERNEL) {
                    WARN("forged EXCEPTION_MSG from #%d, ignoring...", m.src);
                    break;
                }

                struct task *task = task_find(m.exception.task);
                if (!task) {
                    WARN("unknown task %d", m.exception.task);
                    break;
                }

                switch (m.exception.reason) {
                    case EXP_GRACE_EXIT:
                        task_destroy(task);
                        TRACE("%s exited gracefully", task->name);
                        break;
                    case EXP_INVALID_UADDR:
                        ERROR("%s: invalid uaddr", task->name);
                        break;
                    case EXP_INVALID_PAGER_REPLY:
                        ERROR("unexpected exception type %d",
                              m.exception.reason);
                        break;
                    default:
                        WARN("unknown exception type %d", m.exception.reason);
                        break;
                }
                break;
            }
            case PAGE_FAULT_MSG: {
                if (m.src != FROM_KERNEL) {
                    WARN("forged PAGE_FAULT_MSG from #%d, ignoring...", m.src);
                    break;
                }

                struct task *task = task_find(m.page_fault.task);
                ASSERT(task);
                ASSERT(task->pager == task_self());
                ASSERT(m.page_fault.task == task->tid);

                error_t err =
                    handle_page_fault(task, m.page_fault.uaddr, m.page_fault.ip,
                                      m.page_fault.fault);
                if (IS_ERROR(err)) {
                    task_destroy(task);
                    break;
                }

                m.type = PAGE_FAULT_REPLY_MSG;
                ipc_reply(task->tid, &m);
                break;
            }
            default:
                WARN("unknown message type: %s from %d", msgtype2str(m.type),
                     m.src);
        }
    }
}
