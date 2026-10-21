#include "gpio.h"

void gpio_set_mode(GPIO_TypeDef *port, uint32_t pin, enum gpio_mode mode)
{
    port->MODER = (port->MODER & ~(3U << (pin * 2))) | ((uint32_t)mode << (pin * 2));
}

void gpio_set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
    uint32_t idx = pin >> 3;
    uint32_t shift = (pin & 7U) * 4U;
    port->AFR[idx] = (port->AFR[idx] & ~(0xFU << shift)) | (af << shift);
    gpio_set_mode(port, pin, GPIO_AF);
}

void gpio_set_open_drain(GPIO_TypeDef *port, uint32_t pin)
{
    port->OTYPER |= 1U << pin;
}

void gpio_set_pull(GPIO_TypeDef *port, uint32_t pin, enum gpio_pull pull)
{
    port->PUPDR = (port->PUPDR & ~(3U << (pin * 2))) | ((uint32_t)pull << (pin * 2));
}

void gpio_set_speed_high(GPIO_TypeDef *port, uint32_t pin)
{
    port->OSPEEDR |= 3U << (pin * 2);
}

void gpio_toggle(GPIO_TypeDef *port, uint32_t pin)
{
    port->ODR ^= 1U << pin;
}
