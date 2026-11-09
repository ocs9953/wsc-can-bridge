
TARGET   = WSC_CAN_Bridge
BUILD    = build

GCC_PATH ?= C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin
CC      = $(GCC_PATH)/arm-none-eabi-gcc
OBJCOPY = $(GCC_PATH)/arm-none-eabi-objcopy
SIZE    = $(GCC_PATH)/arm-none-eabi-size

C_SOURCES = \
  Core/Src/main.c \
  Core/Src/can_if.c \
  Core/Src/slcan.c \
  Core/Src/rtt.c \
  Core/Src/stm32u5xx_it.c \
  Core/Src/syscalls.c \
  Core/Src/system_stm32u5xx.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_cortex.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_rcc.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_rcc_ex.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_pwr.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_pwr_ex.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_gpio.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_fdcan.c \
  Drivers/STM32U5xx_HAL_Driver/Src/stm32u5xx_hal_icache.c

ASM_SOURCES = startup_stm32u575xx.s
LDSCRIPT    = STM32U575VGTx_FLASH.ld

MCU     = -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
DEFS    = -DSTM32U575xx -DUSE_HAL_DRIVER
INCLUDES = \
  -ICore/Inc \
  -IDrivers/STM32U5xx_HAL_Driver/Inc \
  -IDrivers/CMSIS/Device/ST/STM32U5xx/Include \
  -IDrivers/CMSIS/Include

CFLAGS  = $(MCU) $(DEFS) $(INCLUDES) -std=gnu11 -O2 -g3 -Wall \
          -ffunction-sections -fdata-sections -MMD -MP
ASFLAGS = $(MCU) -x assembler-with-cpp
LDFLAGS = $(MCU) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
          -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -Wl,--print-memory-usage \
          -Wl,--no-warn-rwx-segments -lc -lm

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

OBJECTS = $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o))) \
          $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

$(BUILD)/%.o: %.c Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.s Makefile
	$(CC) -c $(ASFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

-include $(wildcard $(BUILD)/*.d)

.PHONY: all
