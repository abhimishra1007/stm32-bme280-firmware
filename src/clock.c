/*
 * System clock: HSI (16 MHz) -> PLL -> 84 MHz SYSCLK.
 *   PLLM = 16  -> 1 MHz VCO input
 *   PLLN = 336 -> 336 MHz VCO output
 *   PLLP = 4   -> 84 MHz SYSCLK
 *   PLLQ = 7   -> 48 MHz (USB/SDIO, unused here)
 * AHB = 84 MHz, APB1 = 42 MHz (max), APB2 = 84 MHz.
 */
#include "stm32f401.h"
#include "clock.h"

static volatile uint32_t tick_ms;

void clock_init(void)
{
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) {
    }

    /* 84 MHz at 3.3 V needs 2 flash wait states (RM0368 table 6) */
    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH->ACR & FLASH_ACR_LATENCY_MASK) != FLASH_ACR_LATENCY_2WS) {
    }

    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) {
    }

    RCC->PLLCFGR = (16U << 0)     /* PLLM */
                 | (336U << 6)    /* PLLN */
                 | (1U << 16)     /* PLLP = 4 (encoded 01) */
                 | (0U << 22)     /* PLLSRC = HSI */
                 | (7U << 24);    /* PLLQ */

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_PPRE1_MASK) | RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
    }
}

void systick_init(void)
{
    SYSTICK->LOAD = (SYSCLK_HZ / 1000U) - 1U; /* 1 ms tick */
    SYSTICK->VAL = 0;
    SYSTICK->CTRL = SYSTICK_CTRL_CLKSOURCE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

void SysTick_Handler(void)
{
    tick_ms++;
}

uint32_t millis(void)
{
    return tick_ms;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = tick_ms;
    while ((tick_ms - start) < ms) {
    }
}
