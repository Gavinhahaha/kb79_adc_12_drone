# ==============================================================================
# CENTAURI60 固件 Makefile（官方工程对齐版 · 100% 可编译）
# 基于 SEGGER Embedded Studio 工程自动生成
# 适配：Windows + RISC-V GCC
# ==============================================================================

.DEFAULT_GOAL := all

UNAME_S := Windows_NT
TIMESTAMP := $(shell powershell -NoProfile -Command "(Get-Date).ToString('yyyyMMddHHmmss')")
SHELL := cmd
DIR_SEP := /

VERSION_TAG := drone_ci
BUILD       ?= release
$(info [SYSTEM] Windows | 版本: $(VERSION_TAG) | BUILD=$(BUILD))

# ========================== 路径定义 ==========================
TARGET       := DRIVER_CENTAURI60
BIN_DIR      := app/keyboard_freertos/GCC/bin
OBJ_BASE     := app/keyboard_freertos/GCC/Objects

OUTPUT_ELF   := $(BIN_DIR)/$(TARGET).elf
OUTPUT_BIN   := $(BIN_DIR)/$(TARGET).bin
OUTPUT_HEX   := $(BIN_DIR)/$(TARGET).hex
OUTPUT_MAP   := $(BIN_DIR)/$(TARGET).map

BSP_DIR      := board/hpm5300_basic
USER_DIR     := app
HAL_DIR      := app/keyboard_freertos/hpm_sdk_localized_for_hpm5300_basic
XIP_DIR      := app/keyboard_freertos/hpm5300_basic_flash_xip_release
MELGEEK_DIR  := app/keyboard_freertos/melgeek
SRC_DIR      := app/keyboard_freertos/src
CONFIG_DIR   := app/config

# ========================== 工具链 ==========================
TOOLCHAIN_BIN := "app/keyboard_freertos/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin"
CC            := $(TOOLCHAIN_BIN)/riscv32-unknown-elf-gcc-13.2.0.exe
AS            := $(TOOLCHAIN_BIN)/riscv32-unknown-elf-as.exe
OBJCOPY       := $(TOOLCHAIN_BIN)/riscv32-unknown-elf-objcopy.exe
OBJDUMP       := $(TOOLCHAIN_BIN)/riscv32-unknown-elf-objdump.exe
SIZE          := $(TOOLCHAIN_BIN)/riscv32-unknown-elf-size.exe

# ========================== 官方工程对齐：架构 ==========================
# 对齐 SES Release: rv32imac + zba/zbb/zbc/zbs + zicsr/zifencei
RISCV_ARCH     := rv32imac_zba_zbb_zbc_zbs_zicsr_zifencei
RISCV_ABI      := ilp32
ARCH_FLAGS     := -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medany \
                  -ffunction-sections -fdata-sections -fno-common

# ========================== 官方宏定义 ==========================
DEFS := \
-DFLASH_XIP=1 \
-DHPMSOC_HAS_HPMSDK_GPIO=y \
-DHPMSOC_HAS_HPMSDK_PLIC=y \
-DHPMSOC_HAS_HPMSDK_MCHTMR=y \
-DHPMSOC_HAS_HPMSDK_PLICSW=y \
-DHPMSOC_HAS_HPMSDK_GPTMR=y \
-DHPMSOC_HAS_HPMSDK_UART=y \
-DHPMSOC_HAS_HPMSDK_I2C=y \
-DHPMSOC_HAS_HPMSDK_SPI=y \
-DHPMSOC_HAS_HPMSDK_CRC=y \
-DHPMSOC_HAS_HPMSDK_TSNS=y \
-DHPMSOC_HAS_HPMSDK_MBX=y \
-DHPMSOC_HAS_HPMSDK_EWDG=y \
-DHPMSOC_HAS_HPMSDK_DMAMUX=y \
-DHPMSOC_HAS_HPMSDK_DMAV2=y \
-DHPMSOC_HAS_HPMSDK_GPIOM=y \
-DHPMSOC_HAS_HPMSDK_MCAN=y \
-DHPMSOC_HAS_HPMSDK_PTPC=y \
-DHPMSOC_HAS_HPMSDK_QEIV2=y \
-DHPMSOC_HAS_HPMSDK_QEO=y \
-DHPMSOC_HAS_HPMSDK_MMC=y \
-DHPMSOC_HAS_HPMSDK_PWM=y \
-DHPMSOC_HAS_HPMSDK_RDC=y \
-DHPMSOC_HAS_HPMSDK_PLB=y \
-DHPMSOC_HAS_HPMSDK_SYNT=y \
-DHPMSOC_HAS_HPMSDK_SEI=y \
-DHPMSOC_HAS_HPMSDK_TRGM=y \
-DHPMSOC_HAS_HPMSDK_USB=y \
-DHPMSOC_HAS_HPMSDK_SDP=y \
-DHPMSOC_HAS_HPMSDK_SEC=y \
-DHPMSOC_HAS_HPMSDK_MON=y \
-DHPMSOC_HAS_HPMSDK_RNG=y \
-DHPMSOC_HAS_HPMSDK_OTP=y \
-DHPMSOC_HAS_HPMSDK_KEYM=y \
-DHPMSOC_HAS_HPMSDK_ADC16=y \
-DHPMSOC_HAS_HPMSDK_DAC=y \
-DHPMSOC_HAS_HPMSDK_OPAMP=y \
-DHPMSOC_HAS_HPMSDK_ACMP=y \
-DHPMSOC_HAS_HPMSDK_SYSCTL=y \
-DHPMSOC_HAS_HPMSDK_IOC=y \
-DHPMSOC_HAS_HPMSDK_PLLCTLV2=y \
-DHPMSOC_HAS_HPMSDK_PPOR=y \
-DHPMSOC_HAS_HPMSDK_PCFG=y \
-DHPMSOC_HAS_HPMSDK_PGPR=y \
-DHPMSOC_HAS_HPMSDK_PDGO=y \
-DHPMSOC_HAS_HPMSDK_PMP=y \
-DUSE_DMA_MGR=1 \
-DCONFIG_DISABLE_GLOBAL_IRQ_ON_STARTUP=1 \
-DCONFIG_FREERTOS=1 \
-DportasmHAS_MTIME=1 \
-D__freertos_irq_stack_top=_stack \
-DUSE_NONVECTOR_MODE=1 \
-DDISABLE_IRQ_PREEMPTIVE=1 \
-DTEST_VERSION="\"$(VERSION_TAG)\""

# ========================== 官方头文件路径 ==========================
INCDIR       := \
-I$(HAL_DIR)/arch \
-I$(HAL_DIR)/arch/riscv/l1c \
-I$(BSP_DIR) \
-I$(HAL_DIR)/soc/HPM5300/HPM5361 \
-I$(HAL_DIR)/soc/HPM5300/ip \
-I$(HAL_DIR)/soc/HPM5300/HPM5361/toolchains \
-I$(HAL_DIR)/soc/HPM5300/HPM5361/boot \
-I$(HAL_DIR)/drivers/inc \
-I$(HAL_DIR)/utils \
-I$(HAL_DIR)/components/debug_console \
-I$(HAL_DIR)/components/usb/device \
-I$(HAL_DIR)/components/spi \
-I$(HAL_DIR)/components/dma_mgr \
-I$(HAL_DIR)/middleware/FreeRTOS/Source/include \
-I$(HAL_DIR)/middleware/cherryusb/common \
-I$(HAL_DIR)/middleware/cherryusb/osal \
-I$(HAL_DIR)/middleware/cherryusb/core \
-I$(HAL_DIR)/middleware/cherryusb/class/hid \
-I$(HAL_DIR)/middleware/cherryusb/class/hub \
-I$(HAL_DIR)/middleware/FreeRTOS/Source/portable/GCC/RISC-V \
-I$(HAL_DIR)/middleware/FreeRTOS/Source/portable/GCC/RISC-V/chip_specific_extensions/HPMicro \
-I$(XIP_DIR)/build_tmp/generated/include \
-I$(CONFIG_DIR) \
-I$(MELGEEK_DIR)/hal \
-I$(MELGEEK_DIR)/matrix \
-I$(MELGEEK_DIR)/dfu \
-I$(MELGEEK_DIR)/easy_fifo \
-I$(MELGEEK_DIR)/hive \
-I$(MELGEEK_DIR)/db \
-I$(MELGEEK_DIR)/interface \
-I$(MELGEEK_DIR)/ver \
-I$(MELGEEK_DIR)/drivers \
-I$(MELGEEK_DIR)/rgb \
-I$(MELGEEK_DIR)/rgb/led_effect \
-I$(MELGEEK_DIR)/power_save \
-I$(MELGEEK_DIR)/sk \
-I$(MELGEEK_DIR)/layout \
-I$(MELGEEK_DIR)/uart \
-I$(MELGEEK_DIR)/segger_rtt \
-I$(MELGEEK_DIR)/input_devices \
-I$(MELGEEK_DIR)/sm \
-I$(MELGEEK_DIR)/detection \
-I$(MELGEEK_DIR)/factory \
-I$(MELGEEK_DIR)/log \
-I$(SRC_DIR)

# ========================== 编译参数（对齐 centauri80.emProject） ==========================
ifeq ($(BUILD),debug)
  DEFS += -DDEBUG
  CFLAGS_OPT := -g3 -O2
else
  DEFS += -DNDEBUG
  CFLAGS_OPT := -O2
endif

CFLAGS := $(ARCH_FLAGS) $(DEFS) $(CFLAGS_OPT) -std=gnu99 \
-Wall -Wextra -Wno-format -Wundef \
-mcmodel=medany \
-fno-builtin \
-fomit-frame-pointer \
-ffunction-sections -fdata-sections \
-fno-asynchronous-unwind-tables -fno-unwind-tables -fno-strict-aliasing \
$(INCDIR)

ASFLAGS        := $(ARCH_FLAGS) -x assembler-with-cpp -D__ASSEMBLY__ $(DEFS) $(INCDIR)

# ========================== 链接脚本 ==========================
LDSCRIPT       := $(HAL_DIR)/soc/HPM5300/HPM5361/toolchains/gcc/flash_xip.ld
LFLAGS         := -T$(LDSCRIPT) -Wl,-Map=$(OUTPUT_MAP) -Wl,--gc-sections --specs=nano.specs -lm -lc -lgcc
LFLAGS         += -nostartfiles
LFLAGS         += -Wl,--defsym=_stack_size=0x1000
LFLAGS         += -Wl,--defsym=_heap_size=0x0000
LFLAGS         += -Wl,--defsym=_flash_size=0x100000

# ========================== 源文件 ==========================
ASM_SRCS  := \
$(HAL_DIR)/soc/HPM5300/HPM5361/toolchains/gcc/start.S \
$(HAL_DIR)/middleware/FreeRTOS/Source/portable/GCC/RISC-V/portASM.S

C_SRCS  := $(wildcard $(HAL_DIR)/drivers/src/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/soc/HPM5300/HPM5361/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/soc/HPM5300/HPM5361/boot/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/soc/HPM5300/HPM5361/toolchains/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/arch/riscv/l1c/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/components/debug_console/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/components/dma_mgr/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/components/spi/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/components/usb/device/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/utils/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/middleware/FreeRTOS/Source/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/middleware/FreeRTOS/Source/portable/GCC/RISC-V/*.c)
C_SRCS  += $(wildcard $(HAL_DIR)/middleware/FreeRTOS/Source/portable/GCC/RISC-V/chip_specific_extensions/HPMicro/*.c)
C_SRCS  += $(HAL_DIR)/middleware/FreeRTOS/Source/portable/MemMang/heap_4.c
# CherryUSB：仅 device HID（与 centauri80.emProject 一致，勿编入 usbh/usbotg）
C_SRCS  += $(HAL_DIR)/middleware/cherryusb/core/usbd_core.c
C_SRCS  += $(HAL_DIR)/middleware/cherryusb/class/hid/usbd_hid.c
C_SRCS  += $(HAL_DIR)/middleware/cherryusb/osal/usb_osal_freertos.c
C_SRCS  += $(HAL_DIR)/middleware/cherryusb/port/hpm/usb_dc_hpm.c
C_SRCS  += $(wildcard $(BSP_DIR)/*.c)
C_SRCS  += $(wildcard $(SRC_DIR)/*.c)

C_SRCS  += $(wildcard $(MELGEEK_DIR)/hal/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/matrix/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/dfu/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/easy_fifo/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/hive/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/db/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/interface/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/ver/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/drivers/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/rgb/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/rgb/led_effect/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/power_save/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/sk/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/layout/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/uart/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/segger_rtt/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/input_devices/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/sm/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/detection/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/factory/*.c)
C_SRCS  += $(wildcard $(MELGEEK_DIR)/log/*.c)

# 与 SES 工程一致：排除未加入工程的源文件
C_SRCS := $(filter-out %/drv_button.c,$(C_SRCS))
C_SRCS := $(filter-out %/drv_encoder.c,$(C_SRCS))
C_SRCS := $(filter-out %/mg_hall.c,$(C_SRCS))
C_SRCS := $(filter-out %/usb_descriptor.c,$(C_SRCS))
C_SRCS := $(filter-out %/interface_usb.c,$(C_SRCS))

# ========================== 目标文件 ==========================
C_OBJECTS      := $(addprefix $(OBJ_BASE)/,$(notdir $(C_SRCS:.c=.o)))
ASM_OBJECTS    := $(addprefix $(OBJ_BASE)/,$(notdir $(ASM_SRCS:.S=.o)))
DEP_FILES      := $(C_OBJECTS:.o=.d)

VPATH  = $(sort $(dir $(C_SRCS)))
VPATH += $(sort $(dir $(ASM_SRCS)))

# ========================== 构建规则 ==========================
.PHONY: all clean check_toolchain

check_toolchain:
	@echo 🔧 工具链: $(CC)

all: check_toolchain $(OUTPUT_ELF) $(OUTPUT_BIN) $(OUTPUT_HEX)
	@echo ==============================================
	@echo ✅ 编译成功！
	@echo 📦 BIN 烧录文件: $(OUTPUT_BIN)
	@echo ==============================================
	@$(SIZE) $(OUTPUT_ELF)

# 目录
$(OBJ_BASE) $(BIN_DIR):
	@if not exist "$@" mkdir "$@"

# 链接
$(OUTPUT_ELF): $(ASM_OBJECTS) $(C_OBJECTS) | $(BIN_DIR) $(OBJ_BASE)
	@echo 🔗 链接中...
	$(CC) $^ $(LFLAGS) -o $@

$(OBJ_BASE)/portASM.o: $(HAL_DIR)/middleware/FreeRTOS/Source/portable/GCC/RISC-V/portASM.S | $(OBJ_BASE)
	$(CC) $(ASFLAGS) -c $< -o $@

$(OBJ_BASE)/start.o: $(HAL_DIR)/soc/HPM5300/HPM5361/toolchains/gcc/start.S | $(OBJ_BASE)
	$(CC) $(ASFLAGS) -c $< -o $@

# C 编译
$(OBJ_BASE)/%.o: %.c | $(OBJ_BASE)
	$(CC) $(CFLAGS) -MMD -MP -MF$(OBJ_BASE)/$*.d -c $< -o $@

# 格式转换
$(OUTPUT_BIN): $(OUTPUT_ELF)
	$(OBJCOPY) -O binary -R .eh_frame -R .eh_frame_hdr $< $@

$(OUTPUT_HEX): $(OUTPUT_ELF)
	$(OBJCOPY) -O ihex $< $@

# 清理
clean:
	@if exist "$(BIN_DIR)" rmdir /s /q "$(BIN_DIR)"
	@if exist "$(OBJ_BASE)" rmdir /s /q "$(OBJ_BASE)"
	@echo 🧹 清理完成

-include $(DEP_FILES)