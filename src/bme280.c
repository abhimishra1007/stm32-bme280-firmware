/* BME280 I2C driver: probe, calibration readout, and async sampling. */
#include "bme280.h"
#include "clock.h"

#define I2C_TIMEOUT_MS 20U

static i2c_status_t write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { reg, value };
    return i2c1_transfer_blocking(addr, buf, 2, 0, 0, I2C_TIMEOUT_MS);
}

static i2c_status_t read_regs(uint8_t addr, uint8_t reg, uint8_t *out, uint32_t len)
{
    return i2c1_transfer_blocking(addr, &reg, 1, out, len, I2C_TIMEOUT_MS);
}

i2c_status_t bme280_init(struct bme280 *dev, uint8_t addr)
{
    uint8_t id = 0;
    uint8_t t_block[26];
    uint8_t h_block[7];
    i2c_status_t s;

    dev->addr = addr;

    s = read_regs(addr, BME280_REG_CHIP_ID, &id, 1);
    if (s != I2C_DONE) {
        return s;
    }
    if (id != BME280_CHIP_ID) {
        return I2C_ERR_NACK;
    }

    s = write_reg(addr, BME280_REG_RESET, 0xB6);
    if (s != I2C_DONE) {
        return s;
    }
    delay_ms(5); /* start-up time after soft reset is 2 ms */

    s = read_regs(addr, BME280_REG_CALIB00, t_block, sizeof t_block);
    if (s != I2C_DONE) {
        return s;
    }
    s = read_regs(addr, BME280_REG_CALIB26, h_block, sizeof h_block);
    if (s != I2C_DONE) {
        return s;
    }
    bme280_parse_calib(&dev->calib, t_block, h_block);

    /* Humidity oversampling only takes effect after a write to ctrl_meas,
     * so ctrl_hum goes first. Then: standby 0.5 ms, filter off, and
     * temperature x1, pressure skipped, normal mode. */
    s = write_reg(addr, BME280_REG_CTRL_HUM, 0x01);
    if (s == I2C_DONE) {
        s = write_reg(addr, BME280_REG_CONFIG, 0x00);
    }
    if (s == I2C_DONE) {
        s = write_reg(addr, BME280_REG_CTRL_MEAS, (1U << 5) | (0U << 2) | 0x3U);
    }
    return s;
}

int bme280_start_read(struct bme280 *dev)
{
    dev->reg = BME280_REG_TEMP_MSB; /* 0xFA..0xFE: temperature then humidity */
    return i2c1_start_transfer(dev->addr, &dev->reg, 1, dev->raw, sizeof dev->raw);
}

void bme280_finish_read(const struct bme280 *dev, struct bme280_reading *out)
{
    bme280_convert(&dev->calib, dev->raw, out);
}
