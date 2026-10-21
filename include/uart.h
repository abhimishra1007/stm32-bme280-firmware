#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart2_init(uint32_t baud);
void uart2_write(const char *s);
void uart2_write_int(int32_t value);
void uart2_write_fixed(int32_t value, uint32_t decimals);
void uart2_write_hex8(uint8_t value);

#endif
