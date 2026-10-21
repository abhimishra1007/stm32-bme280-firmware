/* Host unit tests for the BME280 compensation code. Build with `make test`. */
#include <stdio.h>
#include <stdlib.h>
#include "bme280.h"

static int failures;

#define CHECK_EQ(actual, expected) do { \
    long long a_ = (long long)(actual), e_ = (long long)(expected); \
    if (a_ != e_) { \
        printf("FAIL %s:%d: %s = %lld, expected %lld\n", __FILE__, __LINE__, #actual, a_, e_); \
        failures++; \
    } \
} while (0)

static void test_temperature_datasheet_example(void)
{
    /* Worked example from the Bosch BMP280/BME280 compensation application note:
     * dig_T1 = 27504, dig_T2 = 26435, dig_T3 = -1000, adc_T = 519888 -> 25.08 degC */
    struct bme280_calib c = { .dig_T1 = 27504, .dig_T2 = 26435, .dig_T3 = -1000 };
    int32_t t_fine;
    CHECK_EQ(bme280_compensate_temp(&c, 519888, &t_fine), 2508);
    CHECK_EQ(t_fine, 128422);
}

static void test_parse_calib_nibble_packing(void)
{
    uint8_t t_block[26] = { 0 };
    uint8_t h_block[7] = { 0 };

    t_block[0] = 0x70; t_block[1] = 0x6B;   /* T1 = 0x6B70 = 27504 */
    t_block[2] = 0x43; t_block[3] = 0x67;   /* T2 = 0x6743 = 26435 */
    t_block[4] = 0x18; t_block[5] = 0xFC;   /* T3 = 0xFC18 = -1000 */
    t_block[25] = 75;                       /* H1 */

    h_block[0] = 0x6A; h_block[1] = 0x01;   /* H2 = 362 */
    h_block[2] = 0x00;                      /* H3 */
    h_block[3] = 0x13;                      /* E4 */
    h_block[4] = 0x2A;                      /* E5: low nibble -> H4, high nibble -> H5 */
    h_block[5] = 0x03;                      /* E6 */
    h_block[6] = 0x1E;                      /* H6 = 30 */

    struct bme280_calib c;
    bme280_parse_calib(&c, t_block, h_block);
    CHECK_EQ(c.dig_T1, 27504);
    CHECK_EQ(c.dig_T2, 26435);
    CHECK_EQ(c.dig_T3, -1000);
    CHECK_EQ(c.dig_H1, 75);
    CHECK_EQ(c.dig_H2, 362);
    CHECK_EQ(c.dig_H3, 0);
    CHECK_EQ(c.dig_H4, (0x13 << 4) | 0x0A);
    CHECK_EQ(c.dig_H5, (0x03 << 4) | 0x02);
    CHECK_EQ(c.dig_H6, 30);

    /* Negative 12-bit H4 is sign-extended from the 0xE4 byte */
    h_block[3] = 0xFF;
    h_block[4] = 0x0F;
    bme280_parse_calib(&c, t_block, h_block);
    CHECK_EQ(c.dig_H4, -1);
}

static void test_humidity_is_clamped(void)
{
    struct bme280_calib c = { .dig_T1 = 27504, .dig_T2 = 26435, .dig_T3 = -1000,
                              .dig_H1 = 75, .dig_H2 = 362, .dig_H3 = 0,
                              .dig_H4 = 314, .dig_H5 = 50, .dig_H6 = 30 };
    int32_t t_fine;
    bme280_compensate_temp(&c, 519888, &t_fine);

    uint32_t low = bme280_compensate_hum(&c, 0, t_fine);
    uint32_t high = bme280_compensate_hum(&c, 0xFFFF, t_fine);
    CHECK_EQ(low, 0);
    CHECK_EQ(high <= 100U * 1024U, 1);

    /* Mid-scale raw value should land somewhere physically sensible */
    uint32_t mid = bme280_compensate_hum(&c, 0x6000, t_fine);
    CHECK_EQ(mid > 0 && mid < 100U * 1024U, 1);
}

static void test_convert_unpacks_20_bit_temperature(void)
{
    struct bme280_calib c = { .dig_T1 = 27504, .dig_T2 = 26435, .dig_T3 = -1000 };
    /* adc_T = 519888 = 0x7EED0 -> msb 0x7E, lsb 0xED, xlsb 0x00 (upper nibble) */
    uint8_t raw[5] = { 0x7E, 0xED, 0x00, 0x00, 0x00 };
    struct bme280_reading r;
    bme280_convert(&c, raw, &r);
    CHECK_EQ(r.temp_centi_c, 2508);
}

int main(void)
{
    test_temperature_datasheet_example();
    test_parse_calib_nibble_packing();
    test_humidity_is_clamped();
    test_convert_unpacks_20_bit_temperature();

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("All BME280 compensation tests passed\n");
    return EXIT_SUCCESS;
}
