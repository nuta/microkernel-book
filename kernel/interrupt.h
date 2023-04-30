#pragma once
#include <libs/common/types.h>

extern unsigned uptime_ticks;

struct task;
error_t irq_listen(struct task *task, unsigned irq);
error_t irq_unlisten(struct task *task, unsigned irq);
void handle_interrupt(unsigned irq);
void handle_timer_interrupt(unsigned ticks);
