#ifndef BME280_H
#define BME280_H

#include <stdint.h>
#include "i2c.h"

#define BME280_ADDR_PRIMARY   0x76 /* SDO tied to GND */
#define BME280_ADDR_SECONDARY 0x77 /* SDO tied to VDDIO */
#define BME280_CHIP_ID        0x60

#define BME280_REG_CALIB00    0x88
#define BME280_REG_CHIP_ID    0xD0
#define BME280_REG_RESET      0xE0
#define BME280_REG_CALIB26    0xE1
#define BME280_REG_CTRL_HUM   0xF2
#define BME280_REG_STATUS     0xF3
#define BME280_REG_CTRL_MEAS  0xF4
#define BME280_REG_CONFIG     0xF5
#define BME280_REG_TEMP_MSB   0xFA

/* Factory trimming parameters (datasheet section 4.2.2) */
struct bme280_calib {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
};

struct bme280_reading {
    int32_t  temp_centi_c;   /* 0.01 degC */
    uint32_t hum_milli_pct;  /* 0.001 %RH */
};

/* Pure functions, no hardware access (unit-tested on the host). */
void bme280_parse_calib(struct bme280_calib *c, const uint8_t t_block[26], const uint8_t h_block[7]);
int32_t bme280_compensate_temp(const struct bme280_calib *c, int32_t adc_T, int32_t *t_fine);
uint32_t bme280_compensate_hum(const struct bme280_calib *c, int32_t adc_H, int32_t t_fine);
void bme280_convert(const struct bme280_calib *c, const uint8_t raw[5], struct bme280_reading *out);

/* Driver */
struct bme280 {
    uint8_t addr;
    struct bme280_calib calib;
    uint8_t reg;          /* register pointer for async reads */
    uint8_t raw[5];       /* temp_msb, temp_lsb, temp_xlsb, hum_msb, hum_lsb */
};

i2c_status_t bme280_init(struct bme280 *dev, uint8_t addr);
int bme280_start_read(struct bme280 *dev);
void bme280_finish_read(const struct bme280 *dev, struct bme280_reading *out);

#endif
