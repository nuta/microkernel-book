#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

#include <libs/common/print.h>

// functions declared in platform_api_vmcore.h
int bh_platform_init(void) {
    return 0;
}

void bh_platform_destroy(void) {}

void *os_malloc(unsigned size) {
    return NULL;
}

void *os_realloc(void *ptr, unsigned size) {
    return NULL;
}

void os_free(void *ptr) {}

int os_printf(const char *fortmat, ...) {
    va_list vargs;
    va_start(vargs, fortmat);
    vprintf(fortmat, vargs);
    va_end(vargs);
    return 0;
}

int os_vprintf(const char *format, va_list ap) {
    vprintf(format, ap);
    return 0;
}

uint64 os_time_get_boot_microsecond(void) {
    return 0;
}

korp_tid os_self_thread(void) {
    return 0;
}

uint8 *os_thread_get_stack_boundary(void) {
    return NULL;
}

void os_thread_jit_write_protect_np(bool enabled) {}

int os_mutex_init(korp_mutex *mutex) {
    return 0;
}

int os_mutex_destroy(korp_mutex *mutex) {
    return 0;
}

int os_mutex_lock(korp_mutex *mutex) {
    return 0;
}

int os_mutex_unlock(korp_mutex *mutex) {
    return 0;
}

void *os_mmap(void *hint, size_t size, int prot, int flags, os_file_handle file) {
    return NULL;
}

void os_munmap(void *addr, size_t size) {}

int os_mprotect(void *addr, size_t size, int prot) {
    return 0;
}

void os_dcache_flush(void) {}

void os_icache_flush(void *start, size_t len) {}

// functions declared in platform_api_extension.h
int os_cond_init(korp_cond *cond) {
    NYI();
    return 0;
}

int os_cond_destroy(korp_cond *cond) {
    NYI();
    return 0;
}

int os_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, uint64 useconds) {
    NYI();
    return 0;
}

int os_cond_signal(korp_cond *cond) {
    NYI();
    return 0;
}

int os_dumps_proc_mem_info(char *out, unsigned int size) {
    NYI();
    return 0;
}

// functions used not provided by os and used in the WAMR source code.
void abort(void){
    NYI();
}

unsigned long int strtoul(const char *nptr, char **endptr, int base) {
    NYI();
    return 0;
}

unsigned long long int strtoull(const char *nptr, char **endptr, int base) {
    NYI();
    return 0;
}

double strtod(const char *nptr, char **endptr) {
    NYI();
    return 0;
}

float strtof(const char *nptr, char **endptr) {
    NYI();
    return 0;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    NYI();
    return 0;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    NYI();
    return 0;
}