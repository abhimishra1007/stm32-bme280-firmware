#ifndef GPIO_H
#define GPIO_H

#include "stm32f401.h"

enum gpio_mode { GPIO_INPUT = 0, GPIO_OUTPUT = 1, GPIO_AF = 2, GPIO_ANALOG = 3 };
enum gpio_pull { GPIO_NOPULL = 0, GPIO_PULLUP = 1, GPIO_PULLDOWN = 2 };

void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, enum gpio_mode mode);
void gpio_set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af);
void gpio_set_open_drain(GPIO_TypeDef *port, uint32_t pin);
void gpio_set_pull(GPIO_TypeDef *port, uint32_t pin, enum gpio_pull pull);
void gpio_set_speed_high(GPIO_TypeDef *port, uint32_t pin);
void gpio_toggle(GPIO_TypeDef *port, uint32_t pin);

#endif
