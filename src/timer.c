/* TIM2 as a periodic sample trigger. */
#include "stm32f401.h"
#include "clock.h"
#include "timer.h"

static volatile uint32_t pending_ticks;

void tim2_init_periodic(uint32_t rate_hz)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 = 0;
    TIM2->PSC = (TIM2_CLK_HZ / 10000U) - 1U;   /* 10 kHz counter clock */
    TIM2->ARR = (10000U / rate_hz) - 1U;
    TIM2->EGR = TIM_EGR_UG;                    /* load PSC/ARR now */
    TIM2->SR = 0;
    TIM2->DIER = TIM_DIER_UIE;

    nvic_enable(TIM2_IRQn, 2);
    TIM2->CR1 = TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR = ~TIM_SR_UIF; /* rc_w0: write 0 to clear */
        pending_ticks++;
    }
}

int tim2_consume_tick(void)
{
    if (pending_ticks == 0U) {
        return 0;
    }
    __disable_irq();
    pending_ticks--;
    __enable_irq();
    return 1;
}
