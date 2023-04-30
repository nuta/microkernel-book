#pragma once
#include <libs/common/print.h>

void handle_serial_interrupt(void);
int serial_read(char *buf, int max_len);
