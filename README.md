# Bare-Metal BME280 Sensor Firmware (STM32F401RE)

Register-level C firmware for the STM32 Nucleo-F401RE (ARM Cortex-M4) that reads
temperature and humidity from a Bosch BME280 over I2C and streams the readings over
UART at 10 Hz. No vendor HAL and no CMSIS: clocks, GPIO, timers, NVIC, I2C, and UART
are configured directly from the STM32F401 reference manual (RM0368).

> Needs the hardware to run: a Nucleo-F401RE and a BME280 breakout board.
> The compensation math has host-side unit tests that run on any PC.

## What it does

- **Clock setup**: HSI 16 MHz -> PLL -> 84 MHz SYSCLK, APB1 at 42 MHz, 2 flash wait states.
- **SysTick**: 1 ms tick for delays and timeouts.
- **TIM2**: update interrupt at 10 Hz that triggers each sample.
- **I2C1 driver**: interrupt-driven master (event and error IRQs) at 100 kHz, with the
  1-byte / 2-byte / N-byte receive sequences the F4 I2C peripheral requires,
  NACK / arbitration-lost / bus-error reporting, and bus recovery (9 SCL pulses + STOP)
  for a slave stuck holding SDA low.
- **BME280 driver**: chip-ID probe at 0x76 and 0x77, soft reset, factory calibration
  readout, normal mode with x1 temperature and humidity oversampling, and Bosch's
  32-bit integer compensation formulas (no FPU needed).
- **USART2**: 115200 8N1 on the ST-LINK virtual COM port, interrupt-driven TX ring buffer.
- **Main loop**: sleeps in `WFI`; each TIM2 tick starts an async 5-byte burst read, and
  completed readings are printed. LD2 (PA5) blinks as a heartbeat.

## Wiring

| BME280 | Nucleo-F401RE |
|--------|---------------|
| VIN    | 3V3           |
| GND    | GND           |
| SCL    | D15 (PB8)     |
| SDA    | D14 (PB9)     |

Most breakout boards include I2C pull-ups. The firmware also enables the internal ones,
which are weak (~40 kΩ), so add external 4.7 kΩ pull-ups if your board has none.

## Build and flash

Requires `arm-none-eabi-gcc` and, for flashing, [stlink](https://github.com/stlink-org/stlink).

```sh
make            # builds build/bme280_logger.elf and .bin
make flash      # writes the .bin to the board over ST-LINK
make test       # runs the compensation unit tests on the host with gcc
```

You can also drag `build/bme280_logger.bin` onto the NUCLEO USB drive to flash it.

Open the virtual COM port at 115200 baud (`screen /dev/ttyACM0 115200`, PuTTY, etc.):

```
BME280 bare-metal logger (STM32F401RE @ 84 MHz)
BME280 found at 0x76
0,T=23.41C,H=38.212%
1,T=23.41C,H=38.207%
...
```

If the sensor doesn't answer, the firmware prints the I2C error (`NACK`, `bus error`,
`timeout`) and retries every second.

## Project layout

```
include/stm32f401.h      hand-written register map (RCC, FLASH, GPIO, USART, I2C, TIM, SysTick, NVIC)
src/startup.c            vector table and reset handler (.data copy, .bss clear)
src/clock.c              PLL to 84 MHz, SysTick
src/gpio.c               pin mode / alternate function helpers
src/uart.c               USART2 with interrupt-driven TX
src/timer.c              TIM2 periodic interrupt
src/i2c.c                interrupt-driven I2C1 master + bus recovery
src/bme280.c             sensor init and async reads
src/bme280_compensate.c  calibration parsing and compensation math
src/main.c               10 Hz sampling loop
test/test_bme280.c       host unit tests
STM32F401RE.ld           linker script (512 KB flash, 96 KB RAM)
```

## References

- RM0368: STM32F401xB/C and STM32F401xD/E reference manual
- Bosch BME280 datasheet (BST-BME280-DS002)
- UM1724: STM32 Nucleo-64 boards user manual
