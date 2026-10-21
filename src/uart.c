/*
 * USART2 on PA2 (TX) / PA3 (RX), AF7. On the Nucleo-F401RE these pins are
 * wired to the ST-LINK virtual COM port, so output shows up over USB.
 *
 * Transmit is interrupt driven: writers push into a ring buffer and the
 * TXE interrupt drains it, so the main loop never blocks on the UART.
 */
#include "stm32f401.h"
#include "gpio.h"
#include "clock.h"
#include "uart.h"

#define TX_BUF_SIZE 256U /* power of two */

static volatile uint8_t tx_buf[TX_BUF_SIZE];
static volatile uint32_t tx_head; /* written by main code */
static volatile uint32_t tx_tail; /* written by ISR */

void uart2_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    gpio_set_af(GPIOA, 2, 7);
    gpio_set_af(GPIOA, 3, 7);
    gpio_set_pull(GPIOA, 3, GPIO_PULLUP);

    /* Oversampling by 16: BRR = fCK / baud (mantissa and fraction packed together) */
    USART2->BRR = (APB1_HZ + baud / 2U) / baud;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    nvic_enable(USART2_IRQn, 3);
}

static void uart2_putc(char c)
{
    uint32_t next = (tx_head + 1U) & (TX_BUF_SIZE - 1U);
    while (next == tx_tail) {
        /* buffer full: wait for the ISR to make room */
    }
    tx_buf[tx_head] = (uint8_t)c;
    tx_head = next;
    USART2->CR1 |= USART_CR1_TXEIE;
}

void USART2_IRQHandler(void)
{
    if ((USART2->SR & USART_SR_TXE) && (USART2->CR1 & USART_CR1_TXEIE)) {
        if (tx_tail != tx_head) {
            USART2->DR = tx_buf[tx_tail];
            tx_tail = (tx_tail + 1U) & (TX_BUF_SIZE - 1U);
        } else {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        }
    }
}

void uart2_write(const char *s)
{
    while (*s) {
        uart2_putc(*s++);
    }
}

void uart2_write_int(int32_t value)
{
    char buf[12];
    uint32_t i = 0;
    uint32_t v;

    if (value < 0) {
        uart2_putc('-');
        v = (uint32_t)(-(value + 1)) + 1U;
    } else {
        v = (uint32_t)value;
    }
    do {
        buf[i++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v != 0U);
    while (i > 0U) {
        uart2_putc(buf[--i]);
    }
}

/* Prints value / 10^decimals, e.g. (2508, 2) -> "25.08" */
void uart2_write_fixed(int32_t value, uint32_t decimals)
{
    int32_t scale = 1;
    for (uint32_t i = 0; i < decimals; i++) {
        scale *= 10;
    }
    if (value < 0) {
        uart2_putc('-');
        value = -value;
    }
    uart2_write_int(value / scale);
    if (decimals > 0U) {
        int32_t frac = value % scale;
        uart2_putc('.');
        for (int32_t d = scale / 10; d > 0; d /= 10) {
            uart2_putc((char)('0' + (frac / d) % 10));
        }
    }
}

void uart2_write_hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    uart2_putc('0');
    uart2_putc('x');
    uart2_putc(hex[value >> 4]);
    uart2_putc(hex[value & 0xFU]);
}
