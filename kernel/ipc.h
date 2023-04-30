#pragma once
#include <libs/common/types.h>

struct task;
struct message;
error_t ipc(struct task *dst, task_t src, __user struct message *m,
            unsigned flags);
void notify(struct task *dst, notifications_t notifications);
