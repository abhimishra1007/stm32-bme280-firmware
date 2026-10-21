/*
 * Interrupt-driven I2C1 master driver, 100 kHz standard mode.
 *   PB8 = SCL (Arduino D15), PB9 = SDA (Arduino D14), AF4, open drain.
 *
 * The STM32F4 I2C peripheral needs a specific register sequence around
 * the ADDR event when receiving 1 or 2 bytes (RM0368 section 18.3.3), so the
 * receive path handles three cases:
 *   N == 1 : clear ACK and set STOP before clearing ADDR.
 *   N == 2 : set POS, clear ACK, clear ADDR, then wait for BTF and read both.
 *   N >= 3 : ACK each byte on RXNE; after reading byte N-1 clear ACK and set
 *            STOP so the final byte is NACKed.
 */
#include "stm32f401.h"
#include "gpio.h"
#include "clock.h"
#include "i2c.h"

enum phase { PHASE_IDLE, PHASE_TX, PHASE_RESTART, PHASE_RX };

static struct {
    volatile enum phase phase;
    volatile i2c_status_t status;
    uint8_t addr;
    const uint8_t *tx;
    uint32_t tx_len;
    volatile uint32_t tx_idx;
    uint8_t *rx;
    uint32_t rx_len;
    volatile uint32_t rx_idx;
} xfer;

static void i2c1_configure(void)
{
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;

    I2C1->CR2 = (uint32_t)(APB1_HZ / 1000000UL);             /* FREQ = 42 MHz */
    I2C1->CCR = (uint32_t)(APB1_HZ / (2UL * 100000UL));     /* Sm: Thigh = Tlow = CCR * Tpclk1 */
    I2C1->TRISE = (uint32_t)(APB1_HZ / 1000000UL) + 1U;     /* 1000 ns max rise time in Sm */
    I2C1->CR1 = I2C_CR1_PE;
}

static void i2c1_pins_af(void)
{
    for (uint32_t pin = 8; pin <= 9; pin++) {
        gpio_set_open_drain(GPIOB, pin);
        gpio_set_pull(GPIOB, pin, GPIO_PULLUP);
        gpio_set_speed_high(GPIOB, pin);
        gpio_set_af(GPIOB, pin, 4);
    }
}

void i2c1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    i2c1_pins_af();
    i2c1_configure();

    xfer.phase = PHASE_IDLE;
    xfer.status = I2C_IDLE;

    nvic_enable(I2C1_EV_IRQn, 1);
    nvic_enable(I2C1_ER_IRQn, 1);
}

static void finish(i2c_status_t s)
{
    I2C1->CR2 &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    I2C1->CR1 &= ~I2C_CR1_POS;
    xfer.phase = PHASE_IDLE;
    xfer.status = s;
}

int i2c1_start_transfer(uint8_t addr7, const uint8_t *tx, uint32_t tx_len,
                        uint8_t *rx, uint32_t rx_len)
{
    if (xfer.phase != PHASE_IDLE || (tx_len == 0U && rx_len == 0U)) {
        return -1;
    }
    if (I2C1->SR2 & I2C_SR2_BUSY) {
        xfer.status = I2C_ERR_BUS;
        return -1;
    }

    xfer.addr = addr7;
    xfer.tx = tx;
    xfer.tx_len = tx_len;
    xfer.tx_idx = 0;
    xfer.rx = rx;
    xfer.rx_len = rx_len;
    xfer.rx_idx = 0;
    xfer.status = I2C_BUSY;
    xfer.phase = (tx_len > 0U) ? PHASE_TX : PHASE_RX;

    I2C1->CR1 &= ~I2C_CR1_POS;
    I2C1->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | I2C_CR2_ITBUFEN;
    I2C1->CR1 |= I2C_CR1_START;
    return 0;
}

i2c_status_t i2c1_status(void)
{
    return xfer.status;
}

void I2C1_EV_IRQHandler(void)
{
    uint32_t sr1 = I2C1->SR1;

    if (sr1 & I2C_SR1_SB) {
        /* Reading SR1 then writing DR clears SB */
        if (xfer.phase == PHASE_RESTART) {
            xfer.phase = PHASE_RX;
            I2C1->CR2 |= I2C_CR2_ITBUFEN;
        }
        I2C1->DR = (uint32_t)(xfer.addr << 1) | (xfer.phase == PHASE_RX ? 1U : 0U);
        return;
    }

    if (sr1 & I2C_SR1_ADDR) {
        if (xfer.phase == PHASE_RX) {
            if (xfer.rx_len == 1U) {
                I2C1->CR1 &= ~I2C_CR1_ACK;
                (void)I2C1->SR2;               /* clears ADDR */
                I2C1->CR1 |= I2C_CR1_STOP;
            } else if (xfer.rx_len == 2U) {
                I2C1->CR1 |= I2C_CR1_POS;
                I2C1->CR1 &= ~I2C_CR1_ACK;
                I2C1->CR2 &= ~I2C_CR2_ITBUFEN; /* wait for BTF, not RXNE */
                (void)I2C1->SR2;
            } else {
                I2C1->CR1 |= I2C_CR1_ACK;
                (void)I2C1->SR2;
            }
        } else {
            (void)I2C1->SR2;
        }
        return;
    }

    switch (xfer.phase) {
    case PHASE_TX:
        if ((sr1 & I2C_SR1_TXE) && xfer.tx_idx < xfer.tx_len) {
            I2C1->DR = xfer.tx[xfer.tx_idx++];
            if (xfer.tx_idx == xfer.tx_len) {
                I2C1->CR2 &= ~I2C_CR2_ITBUFEN; /* next event of interest is BTF */
            }
        } else if (sr1 & I2C_SR1_BTF) {
            if (xfer.rx_len > 0U) {
                xfer.phase = PHASE_RESTART;
                I2C1->CR1 |= I2C_CR1_START;
            } else {
                I2C1->CR1 |= I2C_CR1_STOP;
                finish(I2C_DONE);
            }
        }
        break;

    case PHASE_RX:
        if (xfer.rx_len == 2U) {
            if (sr1 & I2C_SR1_BTF) {
                I2C1->CR1 |= I2C_CR1_STOP;
                xfer.rx[0] = (uint8_t)I2C1->DR;
                xfer.rx[1] = (uint8_t)I2C1->DR;
                xfer.rx_idx = 2;
                finish(I2C_DONE);
            }
        } else if (sr1 & I2C_SR1_RXNE) {
            xfer.rx[xfer.rx_idx++] = (uint8_t)I2C1->DR;
            uint32_t remaining = xfer.rx_len - xfer.rx_idx;
            if (remaining == 1U) {
                I2C1->CR1 &= ~I2C_CR1_ACK;
                I2C1->CR1 |= I2C_CR1_STOP;
            } else if (remaining == 0U) {
                finish(I2C_DONE);
            }
        }
        break;

    default:
        /* PHASE_RESTART waits for SB; PHASE_IDLE should never get here */
        break;
    }
}

void I2C1_ER_IRQHandler(void)
{
    uint32_t sr1 = I2C1->SR1;

    if (sr1 & I2C_SR1_AF) {
        /* Address or data byte was NACKed: release the bus */
        I2C1->SR1 = ~I2C_SR1_AF;
        I2C1->CR1 |= I2C_CR1_STOP;
        finish(I2C_ERR_NACK);
    } else if (sr1 & I2C_SR1_ARLO) {
        /* Hardware has already dropped to slave mode; no STOP needed */
        I2C1->SR1 = ~I2C_SR1_ARLO;
        finish(I2C_ERR_ARLO);
    } else if (sr1 & (I2C_SR1_BERR | I2C_SR1_OVR | I2C_SR1_TIMEOUT)) {
        I2C1->SR1 = ~(I2C_SR1_BERR | I2C_SR1_OVR | I2C_SR1_TIMEOUT);
        I2C1->CR1 |= I2C_CR1_STOP;
        finish(I2C_ERR_BUS);
    }
}

i2c_status_t i2c1_transfer_blocking(uint8_t addr7, const uint8_t *tx, uint32_t tx_len,
                                    uint8_t *rx, uint32_t rx_len, uint32_t timeout_ms)
{
    if (i2c1_start_transfer(addr7, tx, tx_len, rx, rx_len) != 0) {
        return I2C_ERR_BUS;
    }
    uint32_t start = millis();
    while (xfer.status == I2C_BUSY) {
        if ((millis() - start) > timeout_ms) {
            i2c1_recover();
            return I2C_ERR_TIMEOUT;
        }
        __wfi();
    }
    return xfer.status;
}

static void short_delay(void)
{
    /* ~5 us at 84 MHz: half an SCL period at 100 kHz */
    for (volatile uint32_t i = 0; i < 70U; i++) {
    }
}

void i2c1_recover(void)
{
    __disable_irq();
    finish(I2C_IDLE);
    I2C1->CR1 = 0;
    __enable_irq();

    /* Drive the lines as open-drain GPIOs. A slave holding SDA low mid-byte
     * releases it after at most 9 clocks; then generate a STOP by hand. */
    GPIOB->BSRR = (1U << 8) | (1U << 9);
    gpio_set_mode(GPIOB, 8, GPIO_OUTPUT);
    gpio_set_mode(GPIOB, 9, GPIO_OUTPUT);

    for (uint32_t i = 0; i < 9U && !(GPIOB->IDR & (1U << 9)); i++) {
        GPIOB->BSRR = 1U << (8 + 16);   /* SCL low */
        short_delay();
        GPIOB->BSRR = 1U << 8;          /* SCL high */
        short_delay();
    }

    GPIOB->BSRR = 1U << (9 + 16);       /* SDA low while SCL high... */
    short_delay();
    GPIOB->BSRR = 1U << 9;              /* ...then SDA high = STOP */
    short_delay();

    i2c1_pins_af();
    i2c1_configure();
}

const char *i2c_status_str(i2c_status_t s)
{
    switch (s) {
    case I2C_IDLE:        return "idle";
    case I2C_BUSY:        return "busy";
    case I2C_DONE:        return "ok";
    case I2C_ERR_NACK:    return "NACK";
    case I2C_ERR_BUS:     return "bus error";
    case I2C_ERR_ARLO:    return "arbitration lost";
    case I2C_ERR_TIMEOUT: return "timeout";
    }
    return "?";
}
