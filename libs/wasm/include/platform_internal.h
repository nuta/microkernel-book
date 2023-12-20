#pragma once

#include <libs/common/types.h>
#include <libs/common/print.h>
#include <libs/common/string.h>

// types and defines not provided by os and used in the WAMR source code.
typedef task_t korp_tid;
typedef int korp_mutex;
typedef int os_file_handle;
typedef int os_raw_file_handle;
typedef int korp_sem;
typedef int korp_rwlock;
typedef int os_dir_stream;
typedef int korp_cond;
typedef uintmax_t intptr_t;

#define static_assert STATIC_ASSERT
#define UINT16_MAX      (65535U)
#define INT32_MAX		(2147483647)
#define INT32_MIN		(-2147483647-1)
#define UINT32_MAX		(4294967295U)
#define INT64_MAX		(9223372036854775807L)
#define INT64_MIN		(-9223372036854775807L-1)
#define UINT64_MAX		(18446744073709551615UL)
#define UINTPTR_MAX     UINT32_MAX

// math functions not provided by os.
// no need to give an implementation, since it is given by WAMR's math.c.
int isnan(double x);
int signbit(double x);
float fabsf(float x);
float ceilf(float x);
float floorf(float x);
float truncf(float x);
float rintf(float x);
float sqrtf(float x);
double fabs(double x);
double ceil(double x);
double floor(double x);
double trunc(double x);
double rint(double x);
double sqrt(double x);

// functions used not provided by os and used in the WAMR source code.
void abort(void);
unsigned long int strtoul(const char *nptr, char **endptr, int base);
unsigned long long int strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);