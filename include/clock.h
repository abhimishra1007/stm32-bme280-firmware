#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

#define SYSCLK_HZ 84000000UL
#define APB1_HZ   42000000UL
#define TIM2_CLK_HZ (2UL * APB1_HZ) /* APB1 prescaler != 1, so timer clock is doubled */

void clock_init(void);
void systick_init(void);
uint32_t millis(void);
void delay_ms(uint32_t ms);

#endif
