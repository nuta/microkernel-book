#pragma once

#define ARGC_MAX 32

struct args {
    char *argv[ARGC_MAX];
    int argc;
};

struct command {
    const char *name;
    const char *help;
    void (*run)(struct args *args);
};

void run_command(struct args *args);
