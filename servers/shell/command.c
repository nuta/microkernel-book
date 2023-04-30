#include "command.h"
#include "fs.h"
#include "http.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>

static void do_echo(struct args *args) {
    for (int i = 1; i < args->argc; i++) {
        printf("%s ", args->argv[i]);
    }
    printf(PRINT_NL);
}

static void do_http(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: http <URL>");
        return;
    }

    http_get(args->argv[1]);
}

static void do_cat(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: cat <PATH>");
        return;
    }

    fs_read(args->argv[1]);
}

static void do_write(struct args *args) {
    if (args->argc != 3) {
        WARN("Usage: write <PATH> <TEXT>");
        return;
    }

    fs_write(args->argv[1], (const uint8_t *) args->argv[2],
             strlen(args->argv[2]));
}

static void do_listdir(struct args *args) {
    const char *path;
    if (args->argc < 2) {
        path = "/";
    } else {
        path = args->argv[1];
    }

    fs_listdir(path);
}

static void do_mkdir(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: mkdir <PATH>");
        return;
    }

    fs_mkdir(args->argv[1]);
}

static void do_delete(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: delete <PATH>");
        return;
    }

    fs_delete(args->argv[1]);
}

static void do_start(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: start <NAME>");
        return;
    }

    // タスクを起動する
    struct message m;
    m.type = SPAWN_TASK_MSG;
    strcpy_safe(m.spawn_task.name, sizeof(m.spawn_task.name), args->argv[1]);
    error_t err = ipc_call(VM_SERVER, &m);
    if (IS_ERROR(err)) {
        WARN("failed to spawn %s: %s", args->argv[0], err2str(err));
    }

    // タスクが終了するまで待つ
    task_t new_task = m.spawn_task_reply.task;
    while (true) {
        struct message m;
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        if (m.type == TASK_DESTROYED_MSG && m.task_destroyed.task == new_task) {
            break;
        }
    }
}

static void do_sleep(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: sleep <SECONDS>");
        return;
    }

    int seconds = atoi(args->argv[1]);
    if (seconds <= 0) {
        WARN("sleep: invalid seconds: %d", seconds);
        return;
    }

    INFO("sleeping for %d seconds", seconds);

    ASSERT_OK(sys_time(seconds * 1000));

    struct message m;
    do {
        ipc_recv(IPC_ANY, &m);
    } while (m.type != NOTIFY_TIMER_MSG);
}

static void do_ping(struct args *args) {
    if (args->argc != 2) {
        WARN("Usage: ping <VALUE>");
        return;
    }

    task_t pong_server = ipc_lookup("pong");

    // pongサーバにメッセージを送信する
    struct message m;
    m.type = PING_MSG;
    m.ping.value = atoi(args->argv[1]);
    ASSERT_OK(ipc_call(pong_server, &m));

    // pongサーバからの応答が想定されたものか確認する
    ASSERT(m.type == PING_REPLY_MSG);
    ASSERT(m.ping_reply.value == 42);
}

static void do_uptime(struct args *args) {
    printf("%d seconds\n", sys_uptime());
}

__noreturn static void do_shutdown(struct args *args) {
    INFO("shutting down...");
    sys_shutdown();
}

static void do_help(struct args *args);
static struct command commands[] = {
    {.name = "help", .run = do_help, .help = "Show this help"},
    {.name = "echo", .run = do_echo, .help = "Print arguments"},
    {.name = "http", .run = do_http, .help = "Fetch a URL"},
    {.name = "cat", .run = do_cat, .help = "Show file contents"},
    {.name = "write", .run = do_write, .help = "Write text to a file"},
    {.name = "ls", .run = do_listdir, .help = "List files in a directory"},
    {.name = "mkdir", .run = do_mkdir, .help = "Create a directory"},
    {.name = "delete", .run = do_delete, .help = "Delete a file or directory"},
    {.name = "start", .run = do_start, .help = "Launch a task from bootfs"},
    {.name = "sleep", .run = do_sleep, .help = "Pause for a while"},
    {.name = "ping", .run = do_ping, .help = "Send a ping to pong server"},
    {.name = "uptime", .run = do_uptime, .help = "Show seconds since boot"},
    {.name = "shutdown", .run = do_shutdown, .help = "Shut down the system"},
    {.name = NULL},
};

static void do_help(struct args *args) {
    struct command *cmd = commands;
    INFO("");
    INFO("Available commands:");
    INFO("");
    while (cmd->name != NULL) {
        INFO("  %s: %s", cmd->name, cmd->help);
        cmd++;
    }
    INFO("");
}

void run_command(struct args *args) {
    if (args->argc == 0) {
        return;
    }

    struct command *cmd = commands;
    while (cmd->name != NULL) {
        if (!strcmp(cmd->name, args->argv[0])) {
            cmd->run(args);
            return;
        }
        cmd++;
    }

    WARN("unknown command: %s", args->argv[0]);
}
