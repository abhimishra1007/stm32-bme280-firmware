# Build for the STM32 Nucleo-F401RE with arm-none-eabi-gcc. No HAL, no CMSIS.
TARGET  := bme280_logger
BUILD   := build

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

MCU     := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS  := $(MCU) -std=c11 -Os -g3 -Wall -Wextra -Werror -ffreestanding \
           -ffunction-sections -fdata-sections -Iinclude
LDFLAGS := $(MCU) -nostdlib -T STM32F401RE.ld -Wl,--gc-sections \
           -Wl,-Map=$(BUILD)/$(TARGET).map -lgcc

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))

.PHONY: all clean flash test

all: $(BUILD)/$(TARGET).bin
	$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) STM32F401RE.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD):
	mkdir -p $(BUILD)

# Flash with the on-board ST-LINK (requires stlink tools)
flash: $(BUILD)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

# Host-side unit test of the compensation math (runs on your PC, no board needed)
test: | $(BUILD)
	gcc -std=c11 -Wall -Wextra -Werror -Iinclude test/test_bme280.c src/bme280_compensate.c -o $(BUILD)/test_bme280
	./$(BUILD)/test_bme280

clean:
	rm -rf $(BUILD)
