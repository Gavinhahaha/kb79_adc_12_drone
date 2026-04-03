# ==============================================================================
# CENTAURI80：命令行调用 SEGGER emBuild 编 IDE 工程，再把 demo.bin 落到 GCC/bin
#   make        增量编译 + copy bin
#   make all    全量重编 + copy bin（emBuild -rebuild）
#   make clean  删除 IDE Output、GCC/bin、GCC/Objects
# ==============================================================================

.DEFAULT_GOAL := build

ifeq ($(OS),Windows_NT)
  HOST_WINDOWS := 1
endif
ifeq ($(HOST_WINDOWS),1)
  SHELL := cmd
else
  SHELL := /bin/sh
endif

VERSION_TAG := drone_ci
BUILD       ?= debug

$(info [SYSTEM] $(if $(HOST_WINDOWS),Windows,Unix-like) | BUILD=$(BUILD) — make=增量编译+copy bin, make all=重编+copy bin)

# ========================== 路径 ==========================
TARGET            := DRIVER_CENTAURI80
BIN_DIR           := app/keyboard_freertos/GCC/bin
GCC_OBJECTS_DIR   := app/keyboard_freertos/GCC/Objects
BIN_DIR_WIN       := $(subst /,\,$(BIN_DIR))
GCC_OBJECTS_WIN   := $(subst /,\,$(GCC_OBJECTS_DIR))
OUTPUT_BIN        := $(BIN_DIR)/$(TARGET).bin
OUTPUT_BIN_WIN    := $(subst /,\,$(OUTPUT_BIN))
OUTPUT_SECRIT     := $(BIN_DIR)/$(TARGET)_secrit.bin

# ========================== 加密 / 打包参数 ==========================
ENCRYPT_SCRIPT    := app/keyboard_freertos/GCC/encrypt_bin.py
ADD_HEADER_SCRIPT := app/keyboard_freertos/GCC/add_header
# hpm_img_util.exe: 官方 EXIP 加密工具（Windows 直接运行，Linux 通过 wine 运行）
HPM_IMG_UTIL      := app/keyboard_freertos/GCC/hpm_img_util.exe
MG_DEV_NAME       := CENTAURI80
MG_DEV_TYPE       := 0

ENCRYPT_KEK       := 1ca7333ade25f2ca7b468fae642445a3
# 与镜像助手一致：HDR=容器相对镜像起点；FW=固件相对容器的配置值（flat 镜像里即应用区从 0 起的偏移，非 0xC00+0x2000）
ENCRYPT_FW_OFFSET := 0x2000
ENCRYPT_HDR_OFFSET:= 0x0c00
ENCRYPT_LOAD_ADDR := 0x80003000
ENCRYPT_ENTRY     := 0x80003000
ENCRYPT_MEM_BASE  := 0x80000000
ENCRYPT_EXIP_START:= 0x80003000
ENCRYPT_EXIP_LEN  := 0x10000
FLASH_CFG_OPT0    := 0xfcf90002
FLASH_CFG_OPT1    := 0x00000006
FLASH_CFG_OPT2    := 0x00001000
FLASH_CFG_OPT3    := 0x00000000

# demo.bin 首字节 LMA = NOR_CFG 0x80000400；.mg_pack_ver 固定 VA 0x80003200 ⇒ 文件偏移 0x2E00
MG_VER_FILE_OFFSET := 11776

# EXIP 区 AES key/nonce：
#   默认（下三行都空）→ 脚本用 os.urandom，每次构建前 0x100 都不同。
#   镜像助手对同一固件多次生成若前 0x100 相同 → 工具侧为界面固定 Key/Nonce（非每次随机），
#   与默认脚本行为不一致；要与助手逐字节对齐请设置 ENCRYPT_FIXED_EXIP_MATERIAL=界面上的 key||nonce。
# 可选：--rng-seed 仅用于脚本自测/回归（与助手无关，除非你刻意对齐同一 seed 流）。
# 例：make build ENCRYPT_RNG_SEED=ci-repro-1
ENCRYPT_RNG_SEED ?=
ifeq ($(strip $(ENCRYPT_RNG_SEED)),)
  ENCRYPT_RNG_ARG :=
else
  ENCRYPT_RNG_ARG := --rng-seed "$(ENCRYPT_RNG_SEED)"
endif

# ---------- 与 HPM 图形工具逐字节对比（仅测试向量，勿用于量产）----------
# 工具里与下面 makefile 变量保持一致：
#   KEK  ← ENCRYPT_KEK（32 hex，界面可写成 1ca7333a,de25f2ca,7b468fae,642445a3）
#   Flash 基地址 / 加密区域起始与长度 ← ENCRYPT_EXIP_*、FLASH_CFG_OPT* 等同工具选项
#   区域 AES Key（32 hex）+ Nonce（16 hex = 8 字节，与脚本 CTR 一致）手工填写，勿点「随机」
# 约定示例（脚本为 48 hex 连续无空格：key||nonce）：
#   AES   = 00112233445566778899aabbccddeeff
#   Nonce = 0102030405060708
# 构建： make build ENCRYPT_FIXED_EXIP_MATERIAL=00112233445566778899aabbccddeeff0102030405060708
# 若设置本变量，会传 --fixed-exip-material，且优先于 ENCRYPT_RNG_SEED（二者勿同时依赖）。
ENCRYPT_FIXED_EXIP_MATERIAL ?=
ifeq ($(strip $(ENCRYPT_FIXED_EXIP_MATERIAL)),)
  ENCRYPT_EXIP_RAND_ARG := $(ENCRYPT_RNG_ARG)
else
  ifneq ($(strip $(ENCRYPT_RNG_SEED)),)
    $(warning ENCRYPT_RNG_SEED ignored: ENCRYPT_FIXED_EXIP_MATERIAL is set)
  endif
  ENCRYPT_EXIP_RAND_ARG := --fixed-exip-material "$(ENCRYPT_FIXED_EXIP_MATERIAL)"
endif

# 可选：与 Windows 工具对不齐密文时试 nonce_low（传给 encrypt_bin.py --ctr-iv-layout）
ENCRYPT_CTR_IV_LAYOUT ?=
ifeq ($(strip $(ENCRYPT_CTR_IV_LAYOUT)),)
  ENCRYPT_CTR_ARG :=
else
  ENCRYPT_CTR_ARG := --ctr-iv-layout $(ENCRYPT_CTR_IV_LAYOUT)
endif

# 与镜像助手「Firmware Info → Size」对齐（当前示例 0x32170；换 demo 后在助手里读 Size 再改）。
# 使用整段 extract：make ENCRYPT_FW_LENGTH=  （并建议 ENCRYPT_APPEND_HPM_TAIL=0）
ENCRYPT_FW_LENGTH ?= 0x32170
# 1：截断后再从 demo.bin 紧跟 fw 尾追加 hdr_offset(0xC00) 字节，总长度贴近 HPM 工具
ENCRYPT_APPEND_HPM_TAIL ?= 1
ifeq ($(strip $(ENCRYPT_FW_LENGTH)),)
  ENCRYPT_FW_LEN_ARG :=
  ENCRYPT_APPEND_ARG :=
else
  ENCRYPT_FW_LEN_ARG := --fw-length "$(ENCRYPT_FW_LENGTH)"
  ifeq ($(strip $(ENCRYPT_APPEND_HPM_TAIL)),1)
    ENCRYPT_APPEND_ARG := --append-hpm-image-tail
  else
    ENCRYPT_APPEND_ARG :=
  endif
endif

XIP_DIR           := app/keyboard_freertos/hpm5300_basic_flash_xip_release
SES_STUDIO_DIR    := $(XIP_DIR)/segger_embedded_studio
SES_PROJECT_FN    := centauri80.emProject
ifeq ($(BUILD),debug)
  SES_CONFIG := Debug
else
  SES_CONFIG := Release
endif
SES_EXE_DIR       := $(SES_STUDIO_DIR)/Output/$(SES_CONFIG)/Exe
SES_DEMO_BIN      := $(SES_EXE_DIR)/demo.bin
SES_DEMO_ELF      := $(SES_EXE_DIR)/demo.elf
SES_SOLUTION_NAME := cherryusb_device_hid_keyboard_freertos
SES_STUDIODIR     ?=

SES_OUTPUT_DIR    := $(SES_STUDIO_DIR)/Output
SES_OUTPUT_WIN    := $(subst /,\,$(SES_OUTPUT_DIR))
SES_DEMO_BIN_WIN  := $(subst /,\,$(SES_DEMO_BIN))
SES_DEMO_ELF_WIN  := $(subst /,\,$(SES_DEMO_ELF))

# ========================== emBuild ==========================
EMBUILD         ?=
ifeq ($(strip $(EMBUILD)),)
ifneq ($(strip $(SEGGER_EMBUILD)),)
EMBUILD         := $(strip $(SEGGER_EMBUILD))
endif
endif
ifeq ($(HOST_WINDOWS),1)
ifeq ($(strip $(EMBUILD)),)
EMBUILD         := $(strip $(shell powershell -NoProfile -Command "$$roots=@('C:\\Program Files\\SEGGER','C:\\Program Files (x86)\\SEGGER'); foreach($$r in $$roots){ if(Test-Path -LiteralPath $$r){ Get-ChildItem -LiteralPath $$r -Directory -ErrorAction SilentlyContinue | ForEach-Object { $$e=Join-Path $$_.FullName 'bin/emBuild.exe'; if(Test-Path -LiteralPath $$e){ Write-Output $$e; exit 0 } } } }"))
endif
else
ifeq ($(strip $(EMBUILD)),)
EMBUILD         := $(strip $(shell command -v emBuild 2>/dev/null))
endif
ifeq ($(strip $(EMBUILD)),)
EMBUILD         := $(strip $(firstword $(sort $(wildcard /opt/SEGGER/*/bin/emBuild) $(wildcard /opt/segger/*/bin/emBuild) $(wildcard $(HOME)/SEGGER/*/bin/emBuild) $(wildcard $(HOME)/segger/*/bin/emBuild))))
endif
endif

ifneq ($(strip $(EMBUILD)),)
ifeq ($(strip $(SES_STUDIODIR)),)
ifeq ($(HOST_WINDOWS),1)
SES_STUDIODIR := $(strip $(shell powershell -NoProfile -Command "Split-Path (Split-Path -LiteralPath '$(EMBUILD)')"))
else
SES_STUDIODIR := $(strip $(shell dirname "$(shell dirname "$(EMBUILD)")"))
endif
endif
endif

SES_EMBUILD_FLAGS :=
ifneq ($(strip $(SES_STUDIODIR)),)
  SES_EMBUILD_FLAGS += -studiodir "$(SES_STUDIODIR)"
endif
SES_EMBUILD_FLAGS += -config "$(SES_CONFIG)" -solution "$(SES_SOLUTION_NAME)"

ifeq ($(HOST_WINDOWS),1)
TOOLCHAIN_BIN := app/keyboard_freertos/toolchains/rv32imac_zicsr_zifencei_multilib_b_ext-win/bin
OBJCOPY       := $(TOOLCHAIN_BIN)/riscv32-unknown-elf-objcopy.exe
else
ifeq ($(origin OBJCOPY),undefined)
OBJCOPY       := $(strip $(shell sh -c 'for t in riscv32-unknown-elf-objcopy riscv32-none-elf-objcopy; do p=$$(command -v "$$t" 2>/dev/null); [ -n "$$p" ] && echo "$$p" && exit 0; done; echo riscv32-unknown-elf-objcopy'))
endif
endif

# ========================== 规则 ==========================
.PHONY: build all clean

ifeq ($(HOST_WINDOWS),1)

# --- make（增量编译 + copy） ---
build:
ifeq ($(strip $(EMBUILD)),)
	@echo [ERROR] 未找到 emBuild。请安装 SEGGER Embedded Studio，或：
	@echo   make EMBUILD^="C:\Program Files\SEGGER\SEGGER Embedded Studio 8.xx\bin\emBuild.exe"
	@exit /b 1
endif
	@echo [SES] 增量编译 $(SES_CONFIG)...
	cd /d "$(SES_STUDIO_DIR)" && "$(EMBUILD)" $(SES_EMBUILD_FLAGS) "$(SES_PROJECT_FN)" || exit /b 1
	@if not exist "$(SES_DEMO_BIN_WIN)" if not exist "$(SES_DEMO_ELF_WIN)" (echo [ERROR] emBuild 未生成 demo.bin / demo.elf。 & exit /b 1)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	@if exist "$(SES_DEMO_BIN_WIN)" (copy /Y "$(SES_DEMO_BIN_WIN)" "$(OUTPUT_BIN_WIN)" && echo [SES] copied $(SES_DEMO_BIN) -^> $(OUTPUT_BIN)) else if exist "$(SES_DEMO_ELF_WIN)" ("$(OBJCOPY)" -O binary -R .eh_frame -R .eh_frame_hdr "$(SES_DEMO_ELF_WIN)" "$(OUTPUT_BIN_WIN)" && echo [SES] objcopy $(SES_DEMO_ELF) -^> $(OUTPUT_BIN)) else (echo [ERROR] 未找到 demo.bin/demo.elf && exit /b 1)
	@echo [ENCRYPT] 生成加密镜像（调用 hpm_img_util）...
	python "$(ENCRYPT_SCRIPT)" --input "$(OUTPUT_BIN)" --output "$(OUTPUT_SECRIT)" --kek "$(ENCRYPT_KEK)" --fw-offset "$(ENCRYPT_FW_OFFSET)" --hdr-offset "$(ENCRYPT_HDR_OFFSET)" --load-addr "$(ENCRYPT_LOAD_ADDR)" --entry "$(ENCRYPT_ENTRY)" --mem-base "$(ENCRYPT_MEM_BASE)" --exip-start "$(ENCRYPT_EXIP_START)" --exip-len "$(ENCRYPT_EXIP_LEN)" --ver-patch-offset "$(MG_VER_FILE_OFFSET)" --flash-opts "$(FLASH_CFG_OPT0),$(FLASH_CFG_OPT1),$(FLASH_CFG_OPT2),$(FLASH_CFG_OPT3)" $(ENCRYPT_EXIP_RAND_ARG) --use-hpm-tool "$(HPM_IMG_UTIL)" || exit /b 1
	@echo [MG] 生成 .mg 固件包...
	python "$(ADD_HEADER_SCRIPT)" --input "$(OUTPUT_SECRIT)" --output "$(BIN_DIR)" --auto-name --ver-from "$(OUTPUT_BIN)" --ver-offset $(MG_VER_FILE_OFFSET) --dev-name "$(MG_DEV_NAME)" --dev-type $(MG_DEV_TYPE) --delete-encrypted "$(OUTPUT_SECRIT)" --rename-plain-to-match "$(OUTPUT_BIN)" || exit /b 1
	@echo ==============================================
	@echo ✅ 编译完成  固件与 .mg 同名（仅后缀 .bin / .mg）
	@echo ==============================================

# --- make all（全量重编 + copy） ---
all:
ifeq ($(strip $(EMBUILD)),)
	@echo [ERROR] 未找到 emBuild。请安装 SEGGER Embedded Studio，或：
	@echo   make EMBUILD^="C:\Program Files\SEGGER\SEGGER Embedded Studio 8.xx\bin\emBuild.exe"
	@exit /b 1
endif
	@echo [SES] 全量重编 $(SES_CONFIG)...
	cd /d "$(SES_STUDIO_DIR)" && "$(EMBUILD)" $(SES_EMBUILD_FLAGS) -rebuild "$(SES_PROJECT_FN)" || exit /b 1
	@if not exist "$(SES_DEMO_BIN_WIN)" if not exist "$(SES_DEMO_ELF_WIN)" (echo [ERROR] emBuild 未生成 demo.bin / demo.elf。 & exit /b 1)
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	@if exist "$(SES_DEMO_BIN_WIN)" (copy /Y "$(SES_DEMO_BIN_WIN)" "$(OUTPUT_BIN_WIN)" && echo [SES] copied $(SES_DEMO_BIN) -^> $(OUTPUT_BIN)) else if exist "$(SES_DEMO_ELF_WIN)" ("$(OBJCOPY)" -O binary -R .eh_frame -R .eh_frame_hdr "$(SES_DEMO_ELF_WIN)" "$(OUTPUT_BIN_WIN)" && echo [SES] objcopy $(SES_DEMO_ELF) -^> $(OUTPUT_BIN)) else (echo [ERROR] 未找到 demo.bin/demo.elf && exit /b 1)
	@echo [ENCRYPT] 生成加密镜像（调用 hpm_img_util）...
	python "$(ENCRYPT_SCRIPT)" --input "$(OUTPUT_BIN)" --output "$(OUTPUT_SECRIT)" --kek "$(ENCRYPT_KEK)" --fw-offset "$(ENCRYPT_FW_OFFSET)" --hdr-offset "$(ENCRYPT_HDR_OFFSET)" --load-addr "$(ENCRYPT_LOAD_ADDR)" --entry "$(ENCRYPT_ENTRY)" --mem-base "$(ENCRYPT_MEM_BASE)" --exip-start "$(ENCRYPT_EXIP_START)" --exip-len "$(ENCRYPT_EXIP_LEN)" --ver-patch-offset "$(MG_VER_FILE_OFFSET)" --flash-opts "$(FLASH_CFG_OPT0),$(FLASH_CFG_OPT1),$(FLASH_CFG_OPT2),$(FLASH_CFG_OPT3)" $(ENCRYPT_EXIP_RAND_ARG) --use-hpm-tool "$(HPM_IMG_UTIL)" || exit /b 1
	@echo [MG] 生成 .mg 固件包...
	python "$(ADD_HEADER_SCRIPT)" --input "$(OUTPUT_SECRIT)" --output "$(BIN_DIR)" --auto-name --ver-from "$(OUTPUT_BIN)" --ver-offset $(MG_VER_FILE_OFFSET) --dev-name "$(MG_DEV_NAME)" --dev-type $(MG_DEV_TYPE) --delete-encrypted "$(OUTPUT_SECRIT)" --rename-plain-to-match "$(OUTPUT_BIN)" || exit /b 1
	@echo ==============================================
	@echo ✅ 全量重编完成  固件与 .mg 同名（仅后缀 .bin / .mg）
	@echo ==============================================

# --- make clean ---
clean:
	@if exist "$(BIN_DIR_WIN)" rmdir /s /q "$(BIN_DIR_WIN)"
	@if exist "$(GCC_OBJECTS_WIN)" rmdir /s /q "$(GCC_OBJECTS_WIN)"
	@if exist "$(SES_OUTPUT_WIN)" rmdir /s /q "$(SES_OUTPUT_WIN)"
	@echo 已清理 GCC/bin、GCC/Objects、SES IDE Output

else

# --- make（增量编译 + copy） ---
build:
ifeq ($(strip $(EMBUILD)),)
	@echo '[ERROR] 未找到 emBuild。请设置 PATH、EMBUILD 或 SEGGER_EMBUILD。'
	@false
endif
	@echo '[SES] 增量编译 $(SES_CONFIG)...'
	cd "$(SES_STUDIO_DIR)" && "$(EMBUILD)" $(SES_EMBUILD_FLAGS) "$(SES_PROJECT_FN)" || exit 1
	@test -f "$(SES_DEMO_BIN)" || test -f "$(SES_DEMO_ELF)" || (echo '[ERROR] emBuild 未生成 demo.bin/demo.elf' && false)
	mkdir -p "$(BIN_DIR)"
	@if [ -f "$(SES_DEMO_BIN)" ]; then \
		cp -f "$(SES_DEMO_BIN)" "$(OUTPUT_BIN)" && echo "[SES] copied $(SES_DEMO_BIN) -> $(OUTPUT_BIN)"; \
	elif [ -f "$(SES_DEMO_ELF)" ]; then \
		"$(OBJCOPY)" -O binary -R .eh_frame -R .eh_frame_hdr "$(SES_DEMO_ELF)" "$(OUTPUT_BIN)" && echo "[SES] objcopy $(SES_DEMO_ELF) -> $(OUTPUT_BIN)"; \
	else \
		echo "[ERROR] 未找到 demo.bin/demo.elf"; exit 1; \
	fi
	@echo '[ENCRYPT] 生成加密镜像（调用 hpm_img_util via wine）...'
	python3 "$(ENCRYPT_SCRIPT)" \
		--input "$(OUTPUT_BIN)" \
		--output "$(OUTPUT_SECRIT)" \
		--kek "$(ENCRYPT_KEK)" \
		--fw-offset "$(ENCRYPT_FW_OFFSET)" \
		--hdr-offset "$(ENCRYPT_HDR_OFFSET)" \
		--load-addr "$(ENCRYPT_LOAD_ADDR)" \
		--entry "$(ENCRYPT_ENTRY)" \
		--mem-base "$(ENCRYPT_MEM_BASE)" \
		--exip-start "$(ENCRYPT_EXIP_START)" \
		--exip-len "$(ENCRYPT_EXIP_LEN)" \
		--ver-patch-offset "$(MG_VER_FILE_OFFSET)" \
		--flash-opts "$(FLASH_CFG_OPT0),$(FLASH_CFG_OPT1),$(FLASH_CFG_OPT2),$(FLASH_CFG_OPT3)" \
		$(ENCRYPT_EXIP_RAND_ARG) \
		--use-hpm-tool "$(HPM_IMG_UTIL)"
	@echo '[MG] 生成 .mg 固件包...'
	python3 "$(ADD_HEADER_SCRIPT)" --input "$(OUTPUT_SECRIT)" --output "$(BIN_DIR)" \
		--auto-name --ver-from "$(OUTPUT_BIN)" --ver-offset $(MG_VER_FILE_OFFSET) \
		--dev-name "$(MG_DEV_NAME)" --dev-type $(MG_DEV_TYPE) \
		--delete-encrypted "$(OUTPUT_SECRIT)" --rename-plain-to-match "$(OUTPUT_BIN)"
	@echo ==============================================
	@echo '✅ 编译完成  固件与 .mg 同名（仅后缀 .bin / .mg）'
	@echo ==============================================

# --- make all（全量重编 + copy） ---
all:
ifeq ($(strip $(EMBUILD)),)
	@echo '[ERROR] 未找到 emBuild。请设置 PATH、EMBUILD 或 SEGGER_EMBUILD。'
	@false
endif
	@echo '[SES] 全量重编 $(SES_CONFIG)...'
	cd "$(SES_STUDIO_DIR)" && "$(EMBUILD)" $(SES_EMBUILD_FLAGS) -rebuild "$(SES_PROJECT_FN)" || exit 1
	@test -f "$(SES_DEMO_BIN)" || test -f "$(SES_DEMO_ELF)" || (echo '[ERROR] emBuild 未生成 demo.bin/demo.elf' && false)
	mkdir -p "$(BIN_DIR)"
	@if [ -f "$(SES_DEMO_BIN)" ]; then \
		cp -f "$(SES_DEMO_BIN)" "$(OUTPUT_BIN)" && echo "[SES] copied $(SES_DEMO_BIN) -> $(OUTPUT_BIN)"; \
	elif [ -f "$(SES_DEMO_ELF)" ]; then \
		"$(OBJCOPY)" -O binary -R .eh_frame -R .eh_frame_hdr "$(SES_DEMO_ELF)" "$(OUTPUT_BIN)" && echo "[SES] objcopy $(SES_DEMO_ELF) -> $(OUTPUT_BIN)"; \
	else \
		echo "[ERROR] 未找到 demo.bin/demo.elf"; exit 1; \
	fi
	@echo '[ENCRYPT] 生成加密镜像（调用 hpm_img_util via wine）...'
	python3 "$(ENCRYPT_SCRIPT)" \
		--input "$(OUTPUT_BIN)" \
		--output "$(OUTPUT_SECRIT)" \
		--kek "$(ENCRYPT_KEK)" \
		--fw-offset "$(ENCRYPT_FW_OFFSET)" \
		--hdr-offset "$(ENCRYPT_HDR_OFFSET)" \
		--load-addr "$(ENCRYPT_LOAD_ADDR)" \
		--entry "$(ENCRYPT_ENTRY)" \
		--mem-base "$(ENCRYPT_MEM_BASE)" \
		--exip-start "$(ENCRYPT_EXIP_START)" \
		--exip-len "$(ENCRYPT_EXIP_LEN)" \
		--ver-patch-offset "$(MG_VER_FILE_OFFSET)" \
		--flash-opts "$(FLASH_CFG_OPT0),$(FLASH_CFG_OPT1),$(FLASH_CFG_OPT2),$(FLASH_CFG_OPT3)" \
		$(ENCRYPT_EXIP_RAND_ARG) \
		--use-hpm-tool "$(HPM_IMG_UTIL)"
	@echo '[MG] 生成 .mg 固件包...'
	python3 "$(ADD_HEADER_SCRIPT)" --input "$(OUTPUT_SECRIT)" --output "$(BIN_DIR)" \
		--auto-name --ver-from "$(OUTPUT_BIN)" --ver-offset $(MG_VER_FILE_OFFSET) \
		--dev-name "$(MG_DEV_NAME)" --dev-type $(MG_DEV_TYPE) \
		--delete-encrypted "$(OUTPUT_SECRIT)" --rename-plain-to-match "$(OUTPUT_BIN)"
	@echo ==============================================
	@echo '✅ 全量重编完成  固件与 .mg 同名（仅后缀 .bin / .mg）'
	@echo ==============================================

# --- make clean ---
clean:
	rm -rf "$(BIN_DIR)" "$(GCC_OBJECTS_DIR)" "$(SES_OUTPUT_DIR)"
	@echo 已清理 GCC/bin、GCC/Objects、SES IDE Output（若存在）

endif
