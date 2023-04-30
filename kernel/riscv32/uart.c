// シリアルポートドライバ
// https://www.lammertbies.nl/comm/info/serial-uart
#include "uart.h"
#include "asm.h"
#include <kernel/arch.h>

// 文字を送信する
void arch_serial_write(char ch) {
    // 送信可能になるまで待つ
    while ((mmio_read8_paddr(UART_LSR) & UART_LSR_TX_FULL) == 0)
        ;

    // 送信
    mmio_write8_paddr(UART_THR, ch);
}

int arch_serial_read(void) {
    // 受信した文字があるかどうかをチェック
    if ((mmio_read8_paddr(UART_LSR) & UART_LSR_RX_READY) == 0) {
        return -1;
    }

    // 受信した文字を返す
    return mmio_read8_paddr(UART_RBR);
}

// デバイスドライバを初期化する
void riscv32_uart_init(void) {
    // 割り込みを無効にする
    mmio_write8_paddr(UART_IER, 0);

    // FIFOをクリアして有効化する
    mmio_write8_paddr(UART_FCR, UART_FCR_FIFO_ENABLE | UART_FCR_FIFO_CLEAR);

    // 文字を受信したときのみ割り込みを有効にする
    mmio_write8_paddr(UART_IER, UART_IER_RX);
}
