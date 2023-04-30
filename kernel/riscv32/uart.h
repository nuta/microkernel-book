// UARTデバイス
// https://www.lammertbies.nl/comm/info/serial-uart
#pragma once
#include "uart.h"

#define UART_ADDR ((paddr_t) 0x10000000)  // UARTのMMIOの物理アドレス
#define UART0_IRQ 10                      // UARTの割り込み番号

// Transmitter Holding Register.
#define UART_THR (UART_ADDR + 0x00)

// Receiver Buffer Register.
#define UART_RBR (UART_ADDR + 0x00)

/// Interrupt Enable Register.
#define UART_IER    (UART_ADDR + 0x01)
#define UART_IER_RX (1 << 0)

/// FIFO Control Register.
#define UART_FCR (UART_ADDR + 0x02)

/// Line Status Register.
#define UART_LSR          (UART_ADDR + 0x05)
#define UART_LSR_RX_READY (1 << 0)
#define UART_LSR_TX_FULL  (1 << 5)

// FIFO Control Register.
#define UART_FCR_FIFO_ENABLE (1 << 0)
#define UART_FCR_FIFO_CLEAR  (0b11 << 1)

void riscv32_uart_init(void);
