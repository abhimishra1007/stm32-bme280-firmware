/*
 * Bare-metal BME280 logger for the STM32 Nucleo-F401RE.
 *
 * TIM2 fires at 10 Hz. Each tick starts an interrupt-driven I2C read of the
 * BME280 temperature and humidity registers; when the transfer completes the
 * reading is compensated and printed on USART2 (ST-LINK virtual COM port,
 * 115200 8N1). The CPU sleeps in WFI between interrupts.
 *
 * Wiring (BME280 breakout -> Nucleo):
 *   VIN -> 3V3, GND -> GND, SCL -> D15 (PB8), SDA -> D14 (PB9)
 */
#include "stm32f401.h"
#include "clock.h"
#include "gpio.h"
#include "uart.h"
#include "i2c.h"
#include "timer.h"
#include "bme280.h"

#define SAMPLE_RATE_HZ 10U
#define LED_PIN 5U /* LD2 on PA5 */

static struct bme280 sensor;

static void print_error(const char *what, i2c_status_t s)
{
    uart2_write("ERR ");
    uart2_write(what);
    uart2_write(": ");
    uart2_write(i2c_status_str(s));
    uart2_write("\r\n");
}

static uint8_t probe_sensor(void)
{
    static const uint8_t candidates[] = { BME280_ADDR_PRIMARY, BME280_ADDR_SECONDARY };

    for (;;) {
        for (uint32_t i = 0; i < sizeof candidates; i++) {
            i2c_status_t s = bme280_init(&sensor, candidates[i]);
            if (s == I2C_DONE) {
                return candidates[i];
            }
            print_error("BME280 init", s);
            if (s != I2C_ERR_NACK) {
                i2c1_recover();
            }
        }
        delay_ms(1000);
    }
}

int main(void)
{
    clock_init();
    systick_init();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    gpio_set_mode(GPIOA, LED_PIN, GPIO_OUTPUT);

    uart2_init(115200);
    i2c1_init();
    __enable_irq();

    uart2_write("\r\nBME280 bare-metal logger (STM32F401RE @ 84 MHz)\r\n");

    uint8_t addr = probe_sensor();
    uart2_write("BME280 found at ");
    uart2_write_hex8(addr);
    uart2_write("\r\n");

    tim2_init_periodic(SAMPLE_RATE_HZ);

    uint32_t sample = 0;
    int reading = 0;

    for (;;) {
        if (tim2_consume_tick() && !reading) {
            if (bme280_start_read(&sensor) == 0) {
                reading = 1;
            } else {
                print_error("start", i2c1_status());
                i2c1_recover();
            }
        }

        if (reading) {
            i2c_status_t s = i2c1_status();
            if (s == I2C_DONE) {
                struct bme280_reading r;
                bme280_finish_read(&sensor, &r);
                reading = 0;

                uart2_write_int((int32_t)sample++);
                uart2_write(",T=");
                uart2_write_fixed(r.temp_centi_c, 2);
                uart2_write("C,H=");
                uart2_write_fixed((int32_t)r.hum_milli_pct, 3);
                uart2_write("%\r\n");

                if ((sample % SAMPLE_RATE_HZ) == 0U) {
                    gpio_toggle(GPIOA, LED_PIN); /* 0.5 Hz heartbeat */
                }
            } else if (s != I2C_BUSY) {
                reading = 0;
                print_error("read", s);
                if (s != I2C_ERR_NACK) {
                    i2c1_recover();
                }
            }
        }

        __wfi();
    }
}
