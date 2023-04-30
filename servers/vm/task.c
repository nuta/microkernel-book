#include "task.h"
#include "bootfs.h"
#include <libs/common/elf.h>
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/malloc.h>
#include <libs/user/syscall.h>
#include <libs/user/task.h>

static struct task *tasks[NUM_TASKS_MAX];      // タスク管理構造体
static list_t services = LIST_INIT(services);  // サービス管理構造体のリスト

// タスクIDからタスク管理構造体を取得する。
struct task *task_find(task_t tid) {
    if (tid <= 0 || tid > NUM_TASKS_MAX) {
        PANIC("invalid tid %d", tid);
    }

    return tasks[tid - 1];
}

// 指定されたELFファイルからタスクを生成する。成功するとタスクID、失敗するとエラーを返す。
task_t task_spawn(struct bootfs_file *file) {
    TRACE("launching %s...", file->name);
    struct task *task = malloc(sizeof(*task));
    if (!task) {
        PANIC("too many tasks");
    }

    // ELF・プログラムヘッダにアクセスするために、ELFファイルの先頭4096バイトを読み込む。
    void *file_header = malloc(4096);
    bootfs_read(file, 0, file_header, PAGE_SIZE);

    // ELFファイルかチェックする。
    elf_ehdr_t *ehdr = (elf_ehdr_t *) file_header;
    if (memcmp(ehdr->e_ident, ELF_MAGIC, 4) != 0) {
        WARN("%s: invalid ELF magic", file->name);
        free(file_header);
        return ERR_INVALID_ARG;
    }

    // 実行可能ファイルかチェックする。
    if (ehdr->e_type != ET_EXEC) {
        WARN("%s: not an executable file", file->name);
        free(file_header);
        return ERR_INVALID_ARG;
    }

    // プログラムヘッダが多すぎるとfile_headerに収まらないのでエラーにする。32個あれば
    // 十分なはず。
    if (ehdr->e_phnum > 32) {
        WARN("%s: too many program headers", file->name);
        free(file_header);
        return ERR_INVALID_ARG;
    }

    // 新しいタスクをカーネルに生成させる。
    task_t tid_or_err = sys_task_create(file->name, ehdr->e_entry, task_self());
    if (IS_ERROR(tid_or_err)) {
        return tid_or_err;
    }

    // タスク管理構造体を初期化する。
    task->file = file;
    task->file_header = file_header;
    task->tid = tid_or_err;
    task->pager = task_self();
    task->ehdr = ehdr;
    task->phdrs = (elf_phdr_t *) ((uaddr_t) file_header + ehdr->e_phoff);
    task->watch_tasks = false;
    strcpy_safe(task->waiting_for, sizeof(task->waiting_for), "");

    // 仮想アドレス空間のうち空いている仮想アドレス領域の先頭を探す。仮想アドレスを動的に
    // 割り当てる際にELFセグメントと被らないようにするため。
    vaddr_t valloc_next = VALLOC_BASE;
    for (unsigned i = 0; i < task->ehdr->e_phnum; i++) {
        elf_phdr_t *phdr = &task->phdrs[i];
        if (phdr->p_type != PT_LOAD) {
            // メモリ上にないセグメントは無視する。
            continue;
        }

        uaddr_t end = ALIGN_UP(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);
        valloc_next = MAX(valloc_next, end);
    }

    // セグメントの末端が分かったので記録しておく。このアドレスから動的に仮想アドレス領域が
    // 割り当てられていく。
    ASSERT(VALLOC_BASE <= valloc_next && valloc_next < VALLOC_END);
    task->valloc_next = valloc_next;

    // ELFセグメントを仮想アドレス空間にマップする。
    strcpy_safe(task->name, sizeof(task->name), file->name);

    // タスク管理構造体をタスクIDテーブルに登録する。
    tasks[task->tid - 1] = task;
    return task->tid;
}

// タスクを終了させる。
void task_destroy(struct task *task) {
    for (int i = 0; i < NUM_TASKS_MAX; i++) {
        // タスクの終了を監視しているタスクたちに通知する。
        struct task *server = tasks[i];
        if (server && server->watch_tasks) {
            struct message m;
            m.type = TASK_DESTROYED_MSG;
            m.task_destroyed.task = task->tid;
            ipc_send_async(server->tid, &m);
        }
    }

    // タスクをカーネルに終了させる。
    OOPS_OK(sys_task_destroy(task->tid));
    free(task->file_header);
    free(task);

    // タスクIDテーブルからタスク管理構造体を削除する。
    tasks[task_self() - 1] = NULL;
}

// タスクIDを指定してタスクを終了させる。
error_t task_destroy_by_tid(task_t tid) {
    for (int i = 0; i < NUM_TASKS_MAX; i++) {
        struct task *task = tasks[i];
        if (task && task->tid == tid) {
            task_destroy(task);
            return OK;
        }
    }

    return ERR_NOT_FOUND;
}

// サービスを登録する。
void service_register(struct task *task, const char *name) {
    // サービスを登録する。
    struct service *service = malloc(sizeof(*service));
    service->task = task->tid;
    strcpy_safe(service->name, sizeof(service->name), name);
    list_elem_init(&service->next);
    list_push_back(&services, &service->next);
    INFO("service \"%s\" is up", name);

    // このサービスを待っているタスクがいたら、そのタスクに返信して待ち状態を解除してあげる。
    for (int i = 0; i < NUM_TASKS_MAX; i++) {
        struct task *task = tasks[i];
        if (task && !strcmp(task->waiting_for, name)) {
            struct message m;
            m.type = SERVICE_LOOKUP_REPLY_MSG;
            m.service_lookup_reply.task = service->task;
            ipc_reply(task->tid, &m);

            // もう待っていないのでクリアする。
            strcpy_safe(task->waiting_for, sizeof(task->waiting_for), "");
        }
    }
}

// サービス名に対応するタスクIDを返す。まだサービスが登録されていない場合は、ERR_WOULD_BLOCK
// を返す。
task_t service_lookup_or_wait(struct task *task, const char *name) {
    LIST_FOR_EACH (s, &services, struct service, next) {
        if (!strcmp(s->name, name)) {
            return s->task;
        }
    }

    TRACE("%s: waiting for service \"%s\"", task->name, name);
    strcpy_safe(task->waiting_for, sizeof(task->waiting_for), name);
    return ERR_WOULD_BLOCK;
}

// 未だにサービスを待っているタスクがいたら警告を出す。
void service_dump(void) {
    for (int i = 0; i < NUM_TASKS_MAX; i++) {
        struct task *task = tasks[i];
        if (task && strlen(task->waiting_for) > 0) {
            WARN(
                "%s: stil waiting for a service \"%s\""
                " (hint: add the server to BOOT_SERVERS in Makefile)",
                task->name, task->waiting_for);
        }
    }
}
