#pragma once
#include <arch_syscall.h>
#include <libs/common/hinavm_types.h>
#include <libs/common/types.h>

struct message;

error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags);
error_t sys_notify(task_t dst, notifications_t notifications);
task_t sys_task_create(const char *name, vaddr_t ip, task_t pager);
task_t sys_hinavm(const char *name, hinavm_inst_t *insts, size_t num_insts,
                  task_t pager);
error_t sys_task_destroy(task_t task);
__noreturn void sys_task_exit(void);
task_t sys_task_self(void);
pfn_t sys_pm_alloc(task_t tid, size_t size, unsigned flags);
error_t sys_vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, unsigned attrs);
error_t sys_vm_unmap(task_t task, uaddr_t uaddr);
error_t sys_irq_listen(unsigned irq);
error_t sys_irq_unlisten(unsigned irq);
int sys_serial_write(const char *buf, size_t len);
int sys_serial_read(const char *buf, int max_len);
error_t sys_time(int milliseconds);
int sys_uptime(void);
__noreturn void sys_shutdown(void);
