/*
 * Minimal register definitions for the STM32F401RE.
 * Written by hand from RM0368 (STM32F401 reference manual) and the
 * Cortex-M4 generic user guide. Only the peripherals this firmware uses.
 */
#ifndef STM32F401_H
#define STM32F401_H

#include <stdint.h>

#define __IO volatile

/* ---------------- RCC ---------------- */
typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t PLLCFGR;     /* 0x04 */
    __IO uint32_t CFGR;        /* 0x08 */
    __IO uint32_t CIR;         /* 0x0C */
    __IO uint32_t AHB1RSTR;    /* 0x10 */
    __IO uint32_t AHB2RSTR;    /* 0x14 */
    uint32_t      RES0[2];     /* 0x18-0x1C */
    __IO uint32_t APB1RSTR;    /* 0x20 */
    __IO uint32_t APB2RSTR;    /* 0x24 */
    uint32_t      RES1[2];     /* 0x28-0x2C */
    __IO uint32_t AHB1ENR;     /* 0x30 */
    __IO uint32_t AHB2ENR;     /* 0x34 */
    uint32_t      RES2[2];     /* 0x38-0x3C */
    __IO uint32_t APB1ENR;     /* 0x40 */
    __IO uint32_t APB2ENR;     /* 0x44 */
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *)0x40023800UL)

#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)

#define RCC_CFGR_SW_PLL     (2U << 0)
#define RCC_CFGR_SW_MASK    (3U << 0)
#define RCC_CFGR_SWS_PLL    (2U << 2)
#define RCC_CFGR_SWS_MASK   (3U << 2)
#define RCC_CFGR_PPRE1_DIV2 (4U << 10)
#define RCC_CFGR_PPRE1_MASK (7U << 10)

#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_APB1ENR_TIM2EN  (1U << 0)
#define RCC_APB1ENR_USART2EN (1U << 17)
#define RCC_APB1ENR_I2C1EN  (1U << 21)
#define RCC_APB1ENR_PWREN   (1U << 28)
#define RCC_APB1RSTR_I2C1RST (1U << 21)

/* ---------------- FLASH ---------------- */
typedef struct {
    __IO uint32_t ACR;
} FLASH_TypeDef;

#define FLASH ((FLASH_TypeDef *)0x40023C00UL)
#define FLASH_ACR_LATENCY_2WS (2U << 0)
#define FLASH_ACR_LATENCY_MASK (0xFU << 0)
#define FLASH_ACR_PRFTEN    (1U << 8)
#define FLASH_ACR_ICEN      (1U << 9)
#define FLASH_ACR_DCEN      (1U << 10)

/* ---------------- GPIO ---------------- */
typedef struct {
    __IO uint32_t MODER;    /* 0x00 */
    __IO uint32_t OTYPER;   /* 0x04 */
    __IO uint32_t OSPEEDR;  /* 0x08 */
    __IO uint32_t PUPDR;    /* 0x0C */
    __IO uint32_t IDR;      /* 0x10 */
    __IO uint32_t ODR;      /* 0x14 */
    __IO uint32_t BSRR;     /* 0x18 */
    __IO uint32_t LCKR;     /* 0x1C */
    __IO uint32_t AFR[2];   /* 0x20-0x24 */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40020000UL)
#define GPIOB ((GPIO_TypeDef *)0x40020400UL)

/* ---------------- USART ---------------- */
typedef struct {
    __IO uint32_t SR;   /* 0x00 */
    __IO uint32_t DR;   /* 0x04 */
    __IO uint32_t BRR;  /* 0x08 */
    __IO uint32_t CR1;  /* 0x0C */
    __IO uint32_t CR2;  /* 0x10 */
    __IO uint32_t CR3;  /* 0x14 */
    __IO uint32_t GTPR; /* 0x18 */
} USART_TypeDef;

#define USART2 ((USART_TypeDef *)0x40004400UL)
#define USART_SR_TXE    (1U << 7)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_TXEIE (1U << 7)
#define USART_CR1_UE    (1U << 13)

/* ---------------- I2C ---------------- */
typedef struct {
    __IO uint32_t CR1;   /* 0x00 */
    __IO uint32_t CR2;   /* 0x04 */
    __IO uint32_t OAR1;  /* 0x08 */
    __IO uint32_t OAR2;  /* 0x0C */
    __IO uint32_t DR;    /* 0x10 */
    __IO uint32_t SR1;   /* 0x14 */
    __IO uint32_t SR2;   /* 0x18 */
    __IO uint32_t CCR;   /* 0x1C */
    __IO uint32_t TRISE; /* 0x20 */
    __IO uint32_t FLTR;  /* 0x24 */
} I2C_TypeDef;

#define I2C1 ((I2C_TypeDef *)0x40005400UL)

#define I2C_CR1_PE      (1U << 0)
#define I2C_CR1_START   (1U << 8)
#define I2C_CR1_STOP    (1U << 9)
#define I2C_CR1_ACK     (1U << 10)
#define I2C_CR1_POS     (1U << 11)
#define I2C_CR1_SWRST   (1U << 15)

#define I2C_CR2_ITERREN (1U << 8)
#define I2C_CR2_ITEVTEN (1U << 9)
#define I2C_CR2_ITBUFEN (1U << 10)

#define I2C_SR1_SB      (1U << 0)
#define I2C_SR1_ADDR    (1U << 1)
#define I2C_SR1_BTF     (1U << 2)
#define I2C_SR1_RXNE    (1U << 6)
#define I2C_SR1_TXE     (1U << 7)
#define I2C_SR1_BERR    (1U << 8)
#define I2C_SR1_ARLO    (1U << 9)
#define I2C_SR1_AF      (1U << 10)
#define I2C_SR1_OVR     (1U << 11)
#define I2C_SR1_TIMEOUT (1U << 14)

#define I2C_SR2_BUSY    (1U << 1)

/* ---------------- TIM2 (general purpose, 32-bit) ---------------- */
typedef struct {
    __IO uint32_t CR1;   /* 0x00 */
    __IO uint32_t CR2;   /* 0x04 */
    __IO uint32_t SMCR;  /* 0x08 */
    __IO uint32_t DIER;  /* 0x0C */
    __IO uint32_t SR;    /* 0x10 */
    __IO uint32_t EGR;   /* 0x14 */
    __IO uint32_t CCMR1; /* 0x18 */
    __IO uint32_t CCMR2; /* 0x1C */
    __IO uint32_t CCER;  /* 0x20 */
    __IO uint32_t CNT;   /* 0x24 */
    __IO uint32_t PSC;   /* 0x28 */
    __IO uint32_t ARR;   /* 0x2C */
} TIM_TypeDef;

#define TIM2 ((TIM_TypeDef *)0x40000000UL)
#define TIM_CR1_CEN   (1U << 0)
#define TIM_DIER_UIE  (1U << 0)
#define TIM_SR_UIF    (1U << 0)
#define TIM_EGR_UG    (1U << 0)

/* ---------------- Cortex-M4 core: SysTick, NVIC, SCB ---------------- */
typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __IO uint32_t CALIB;
} SysTick_TypeDef;

#define SYSTICK ((SysTick_TypeDef *)0xE000E010UL)
#define SYSTICK_CTRL_ENABLE    (1U << 0)
#define SYSTICK_CTRL_TICKINT   (1U << 1)
#define SYSTICK_CTRL_CLKSOURCE (1U << 2)

#define NVIC_ISER ((__IO uint32_t *)0xE000E100UL)
#define NVIC_IPR  ((__IO uint8_t  *)0xE000E400UL)

/* IRQ numbers (RM0368 table 38) */
#define TIM2_IRQn    28
#define I2C1_EV_IRQn 31
#define I2C1_ER_IRQn 32
#define USART2_IRQn  38

static inline void nvic_enable(uint32_t irq, uint8_t priority)
{
    NVIC_IPR[irq] = (uint8_t)(priority << 4); /* F4 implements 4 priority bits */
    NVIC_ISER[irq >> 5] = 1U << (irq & 31U);
}

static inline void __enable_irq(void)  { __asm volatile ("cpsie i" ::: "memory"); }
static inline void __disable_irq(void) { __asm volatile ("cpsid i" ::: "memory"); }
static inline void __wfi(void)         { __asm volatile ("wfi"); }

#endif /* STM32F401_H */
