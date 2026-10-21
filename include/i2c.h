#ifndef I2C_H
#define I2C_H

#include <stdint.h>

typedef enum {
    I2C_IDLE = 0,
    I2C_BUSY,
    I2C_DONE,
    I2C_ERR_NACK,     /* slave did not acknowledge (AF) */
    I2C_ERR_BUS,      /* misplaced START/STOP, overrun, or bus stuck busy */
    I2C_ERR_ARLO,     /* arbitration lost */
    I2C_ERR_TIMEOUT,  /* transfer did not finish in time */
} i2c_status_t;

void i2c1_init(void);

/*
 * Starts a combined transfer: write tx_len bytes, then (if rx_len > 0) a
 * repeated START and read rx_len bytes. Either length may be 0, not both.
 * Returns 0 on success, -1 if a transfer is already running or the bus is busy.
 * Completion is reported through i2c1_status().
 */
int i2c1_start_transfer(uint8_t addr7, const uint8_t *tx, uint32_t tx_len,
                        uint8_t *rx, uint32_t rx_len);

i2c_status_t i2c1_status(void);

/* Starts a transfer and waits (sleeping between interrupts) until it ends. */
i2c_status_t i2c1_transfer_blocking(uint8_t addr7, const uint8_t *tx, uint32_t tx_len,
                                    uint8_t *rx, uint32_t rx_len, uint32_t timeout_ms);

/* Aborts any transfer, clocks SCL to free a stuck slave, and re-initialises. */
void i2c1_recover(void);

const char *i2c_status_str(i2c_status_t s);

#endif
