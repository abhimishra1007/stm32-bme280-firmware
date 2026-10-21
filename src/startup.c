/*
 * Startup code and vector table for the STM32F401RE.
 * Copies .data from flash to RAM, zeroes .bss, then calls main().
 */
#include <stdint.h>

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Handlers implemented elsewhere; anything not implemented falls back to Default_Handler. */
void NMI_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)      __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void)      __attribute__((weak, alias("Default_Handler")));
void I2C1_EV_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void I2C1_ER_IRQHandler(void)   __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void)    __attribute__((weak, alias("Default_Handler")));

/* 16 core exceptions + 85 peripheral IRQs (RM0368 table 38) */
#define NUM_IRQS 85

__attribute__((section(".isr_vector"), used))
void (*const vector_table[16 + NUM_IRQS])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
    /* External interrupts */
    [16 + 28] = TIM2_IRQHandler,
    [16 + 31] = I2C1_EV_IRQHandler,
    [16 + 32] = I2C1_ER_IRQHandler,
    [16 + 38] = USART2_IRQHandler,
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    for (dst = &_sbss; dst < &_ebss; dst++) {
        *dst = 0;
    }

    main();
    for (;;) {
    }
}

void Default_Handler(void)
{
    for (;;) {
    }
}
