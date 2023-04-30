#pragma once
#include <libs/common/types.h>

struct cpuvar;
#include <arch_types.h>

#define CPUVAR_MAGIC 0xbeefbeef
#define CPUVAR       (arch_cpuvar_get())

struct task;

struct cpuvar {
    struct arch_cpuvar arch;
    int id;
    bool online;
    unsigned ipi_pending;
    struct task *idle_task;
    struct task *current_task;
    unsigned magic;
};

#define IPI_TLB_FLUSH  (1 << 0)
#define IPI_RESCHEDULE (1 << 1)

struct memory_map_entry {
    paddr_t paddr;
    size_t size;
};

#define NUM_MEMORY_MAP_ENTRIES_MAX 8

struct memory_map {
    struct memory_map_entry frees[NUM_MEMORY_MAP_ENTRIES_MAX];
    struct memory_map_entry devices[NUM_MEMORY_MAP_ENTRIES_MAX];
    int num_frees;
    int num_devices;
};

struct bootinfo {
    paddr_t boot_elf;
    struct memory_map memory_map;
};

ARCH_TYPES_STATIC_ASSERTS

void arch_serial_write(char ch);
int arch_serial_read(void);
error_t arch_vm_init(struct arch_vm *vm);
void arch_vm_destroy(struct arch_vm *vm);
error_t arch_vm_map(struct arch_vm *vm, vaddr_t vaddr, paddr_t paddr,
                    unsigned attrs);
error_t arch_vm_unmap(struct arch_vm *vm, vaddr_t vaddr);
vaddr_t arch_paddr_to_vaddr(paddr_t paddr);
bool arch_is_mappable_uaddr(uaddr_t uaddr);
error_t arch_task_init(struct task *task, uaddr_t ip, vaddr_t kernel_entry,
                       void *arg);
void arch_task_destroy(struct task *task);
void arch_task_switch(struct task *prev, struct task *next);
void arch_init(void);
void arch_init_percpu(void);
void arch_idle(void);
void arch_send_ipi(unsigned ipi);
void arch_memcpy_from_user(void *dst, __user const void *src, size_t len);
void arch_memcpy_to_user(__user void *dst, const void *src, size_t len);
error_t arch_irq_enable(unsigned irq);
error_t arch_irq_disable(unsigned irq);
__noreturn void arch_shutdown(void);
