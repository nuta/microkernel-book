#include "command.h"
#include <libs/common/print.h>
#include <libs/common/string.h>
#include <libs/user/ipc.h>
#include <libs/user/syscall.h>

static char *skip_whitespaces(char *p) {
    for (;;) {
        switch (*p) {
            case ' ':
            case '\t':
            case '\n':
                p++;
                continue;
            default:
                return p;
        }
    }
}

static char *consume_argv(char *p) {
    for (;;) {
        switch (*p) {
            case '\0':
            case ';':
                return p;
            case ' ':
            case '\t':
            case '\n':
                *p = '\0';
                p++;
                return p;
            default:
                p++;
        }
    }
}

static void eval(char *script) {
    char *p = script;

    while (*p != '\0') {
        struct args args;
        args.argc = 0;
        do {
            p = skip_whitespaces(p);
            args.argv[args.argc] = p;
            args.argc++;
            p = consume_argv(p);
        } while (*p != '\0' && *p != ';');

        while (*p == ';') {
            *p = '\0';
            p++;
        }

        run_command(&args);
    }
}

static error_t read_line(char *cmdline, int len) {
    int i = 0;
    while (true) {
        char buf[32];
        int read_len = sys_serial_read(buf, sizeof(buf));
        if (read_len < 0) {
            WARN("serial_read failed: %d", read_len);
            return read_len;
        }

        for (int j = 0; j < read_len; j++) {
            if (i >= len) {
                WARN("command line too long");
                return ERR_TOO_LARGE;
            }

            char ch = buf[j];
            switch (ch) {
                case 0x1b:
                    if (j + 2 < read_len) {
                        // 矢印キー入力を無視する
                        j += 2;
                    }
                    continue;
                case 0x7f:
                    if (i > 0) {
                        printf("\b \b");
                        printf_flush();
                        i--;
                    }
                    continue;
                case '\r':
                    ch = '\n';
                    printf("\r");
                    break;
            }

            printf("%c", ch);
            printf_flush();

            if (ch == '\n') {
                cmdline[i] = '\0';
                return OK;
            }

            cmdline[i] = ch;
            i++;
        }
    }
}
void main(void) {
    char cmdline[1024];

    // startコマンドで起動したタスクが終了したら知らせてもらうようにしておく。
    struct message m;
    m.type = WATCH_TASKS_MSG;
    ASSERT_OK(ipc_call(VM_SERVER, &m));

    char *autorun = AUTORUN;
    if (autorun[0] != '\0') {
        strcpy_safe(cmdline, sizeof(cmdline), autorun);
        INFO("running autorun script: %s", cmdline);
        eval(cmdline);
    }

    // 他のサーバが起動しているとシェルのプロンプトがログの中に紛れて分かりづらいので、
    // ここでちょっと待つ。
    sys_time(100);
    ipc_recv(IPC_ANY, &m);
    ASSERT(m.type == NOTIFY_TIMER_MSG);

    printf("\nWelcome to HinaOS!\n\n");
    while (true) {
        printf("\x1b[1mshell> \x1b[0m");
        printf_flush();

        error_t err = read_line(cmdline, sizeof(cmdline));
        if (err == OK) {
            eval(cmdline);
        }
    }
}
