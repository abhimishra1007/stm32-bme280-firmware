/*
 * BME280 compensation formulas, 32-bit integer versions from the Bosch
 * BME280 datasheet (section 4.2.3). No floating point, so this runs on the
 * Cortex-M4 without enabling the FPU.
 */
#include "bme280.h"

static uint16_t u16le(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int16_t  s16le(const uint8_t *p) { return (int16_t)u16le(p); }

void bme280_parse_calib(struct bme280_calib *c, const uint8_t t_block[26], const uint8_t h_block[7])
{
    /* 0x88..0xA1: T1..T3 at 0x88, P1..P9 at 0x8E (unused), H1 at 0xA1 */
    c->dig_T1 = u16le(&t_block[0]);
    c->dig_T2 = s16le(&t_block[2]);
    c->dig_T3 = s16le(&t_block[4]);
    c->dig_H1 = t_block[25];

    /* 0xE1..0xE7: H4 and H5 are 12-bit values sharing the nibbles of 0xE5 */
    c->dig_H2 = s16le(&h_block[0]);
    c->dig_H3 = h_block[2];
    c->dig_H4 = (int16_t)(((int16_t)(int8_t)h_block[3] * 16) | (h_block[4] & 0x0F));
    c->dig_H5 = (int16_t)(((int16_t)(int8_t)h_block[5] * 16) | (h_block[4] >> 4));
    c->dig_H6 = (int8_t)h_block[6];
}

int32_t bme280_compensate_temp(const struct bme280_calib *c, int32_t adc_T, int32_t *t_fine)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)c->dig_T1 << 1))) * ((int32_t)c->dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)c->dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)c->dig_T1))) >> 12) *
                    ((int32_t)c->dig_T3)) >> 14;
    *t_fine = var1 + var2;
    return (*t_fine * 5 + 128) >> 8;
}

/* Returns humidity in Q22.10 %RH (divide by 1024 for %RH). */
uint32_t bme280_compensate_hum(const struct bme280_calib *c, int32_t adc_H, int32_t t_fine)
{
    int32_t v = t_fine - (int32_t)76800;

    v = (((((adc_H << 14) - (((int32_t)c->dig_H4) << 20) - (((int32_t)c->dig_H5) * v)) +
           (int32_t)16384) >> 15) *
         (((((((v * ((int32_t)c->dig_H6)) >> 10) *
              (((v * ((int32_t)c->dig_H3)) >> 11) + (int32_t)32768)) >> 10) +
            (int32_t)2097152) * ((int32_t)c->dig_H2) + 8192) >> 14));
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)c->dig_H1)) >> 4);
    if (v < 0) {
        v = 0;
    }
    if (v > 419430400) {
        v = 419430400;
    }
    return (uint32_t)(v >> 12);
}

void bme280_convert(const struct bme280_calib *c, const uint8_t raw[5], struct bme280_reading *out)
{
    int32_t adc_T = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | ((int32_t)raw[2] >> 4);
    int32_t adc_H = ((int32_t)raw[3] << 8) | (int32_t)raw[4];
    int32_t t_fine;

    out->temp_centi_c = bme280_compensate_temp(c, adc_T, &t_fine);
    uint32_t h_q10 = bme280_compensate_hum(c, adc_H, t_fine);
    out->hum_milli_pct = (h_q10 * 1000U) >> 10;
}
