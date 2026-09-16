# =============================================================================
#  Makefile - bien dich firmware SSL 96-pixel ADB bang GNU Arm Embedded
#             (khong can Keil MDK)
#
#  Cach dung:
#      make            bien dich, tao build/SSL96_ADB_TPS9266x.hex
#      make clean      xoa thu muc build
#      make size       in dung luong FLASH / RAM da dung
#      make flash      nap chip bang ST-LINK_CLI
#
#  Yeu cau:
#      - GNU Arm Embedded Toolchain (arm-none-eabi-gcc) co trong PATH
#        tai ve: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
#      - GNU make (di kem Git for Windows, MSYS2, hoac xpack)
#
#  Ghi chu: chay make TU TRONG thu muc gcc nay. Duong dan deu la duong dan
#  tuong doi vi thu muc goc cua project co dau % va dau cach, de gay loi
#  cho make neu dung duong dan tuyet doi.
# =============================================================================

TARGET  := SSL96_ADB_TPS9266x
BUILD   := build
ROOT    := ..

# ---- Toolchain --------------------------------------------------------------
PREFIX  := arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size

# ---- Ma nguon cua nguoi dung ------------------------------------------------
C_SOURCES := \
  $(ROOT)/User/main.c \
  $(ROOT)/User/tps9266x.c \
  $(ROOT)/User/adb_dimming.c \
  $(ROOT)/User/stm32f10x_it.c \
  $(ROOT)/User/system_stm32f10x.c \
  $(ROOT)/User/uart.c \
  $(ROOT)/User/delay.c \
  $(ROOT)/User/spi.c \
  $(ROOT)/User/I2C.c \
  $(ROOT)/User/Command.c \
  $(ROOT)/User/usbio.c \
  $(ROOT)/User/USB/hw_config.c \
  $(ROOT)/User/USB/usb_desc.c \
  $(ROOT)/User/USB/usb_endp.c \
  $(ROOT)/User/USB/usb_istr.c \
  $(ROOT)/User/USB/usb_prop.c \
  $(ROOT)/User/USB/usb_pwr.c

# ---- Thu vien CMSIS va StdPeriph -------------------------------------------
LIB := $(ROOT)/Libraries

C_SOURCES += \
  $(LIB)/CMSIS/CM3/CoreSupport/core_cm3.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/misc.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_exti.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_rtc.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_flash.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_spi.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_usart.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_tim.c \
  $(LIB)/STM32F10x_StdPeriph_Driver/src/stm32f10x_dma.c

# ---- Thu vien USB FS Device -------------------------------------------------
USBLIB := $(LIB)/STM32_USB-FS-Device_Driver/src

C_SOURCES += \
  $(USBLIB)/usb_core.c \
  $(USBLIB)/usb_init.c \
  $(USBLIB)/usb_int.c \
  $(USBLIB)/usb_mem.c \
  $(USBLIB)/usb_regs.c \
  $(USBLIB)/usb_sil.c

# Bon file otgd_fs_*.c chi danh cho dong STM32F105/107 (STM32F10X_CL) va toan
# bo noi dung nam trong #ifdef STM32F10X_CL, nen khong can bien dich o day.

# ---- File khoi dong ---------------------------------------------------------
ASM_SOURCES := \
  $(LIB)/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/startup_stm32f10x_md.s

# ---- Duong dan header -------------------------------------------------------
# Thu tu giong het Keil: core_cm3.h duoc lay tu DSP_Lib/Include, khong phai
# tu CoreSupport, de trung voi ban dang dung khi bien dich bang Keil.
C_INCLUDES := \
  -I$(ROOT)/User \
  -I$(ROOT)/User/USB/inc \
  -I$(LIB)/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
  -I$(LIB)/STM32F10x_StdPeriph_Driver/inc \
  -I$(LIB)/STM32_USB-FS-Device_Driver/inc \
  -I$(LIB)/DSP_Lib/Include

C_DEFS := -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD

# ---- Co bien dich -----------------------------------------------------------
CPU     := -mcpu=cortex-m3 -mthumb
OPT     := -Og -g3

CFLAGS  := $(CPU) $(C_DEFS) $(C_INCLUDES) $(OPT) \
           -Wall -Wno-unused-parameter \
           -ffunction-sections -fdata-sections \
           -fno-common -fmessage-length=0 \
           -MMD -MP

ASFLAGS := $(CPU) $(OPT) -Wall -MMD -MP

LDSCRIPT := STM32F103C8_FLASH.ld
LDFLAGS  := $(CPU) -T$(LDSCRIPT) \
            --specs=nano.specs --specs=nosys.specs \
            -nostartfiles \
            -Wl,--gc-sections \
            -Wl,-Map=$(BUILD)/$(TARGET).map,--cref \
            -lc -lm -lgcc

# ---- Danh sach file .o ------------------------------------------------------
OBJECTS := $(addprefix $(BUILD)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# ---- Luat -------------------------------------------------------------------
.PHONY: all clean size flash rebuild

all: $(BUILD)/$(TARGET).hex $(BUILD)/$(TARGET).bin size

$(BUILD)/%.o: %.c Makefile | $(BUILD)
	@echo "  CC   $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.s Makefile | $(BUILD)
	@echo "  AS   $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT) Makefile
	@echo "  LD   $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	@echo "  HEX  $@"
	@$(OBJCOPY) -O ihex $< $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	@echo "  BIN  $@"
	@$(OBJCOPY) -O binary -S $< $@

$(BUILD):
	@mkdir -p $@

size: $(BUILD)/$(TARGET).elf
	@echo ""
	@echo "--- Dung luong (FLASH = text + data, RAM = data + bss) ---"
	@$(SIZE) $<
	@echo "  STM32F103C8T6 co 64 KB FLASH va 20 KB RAM"
	@echo ""

rebuild: clean all

clean:
	@echo "  Xoa $(BUILD)"
	@rm -rf $(BUILD)

# Nap chip bang ST-LINK Utility
flash: $(BUILD)/$(TARGET).hex
	ST-LINK_CLI.exe -c SWD UR -P "$(BUILD)/$(TARGET).hex" -V "while_programming" -Rst

-include $(wildcard $(BUILD)/*.d)
