# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

MINI_ERA_BUILD = $(SOFT_BUILD)/mini_era

ARIANE ?= $(ESP_ROOT)/rtl/cores/ariane/ariane

RISCV_TESTS = $(SOFT)/riscv-tests
RISCV_PK = $(SOFT)/riscv-pk
OPENSBI = $(SOFT)/opensbi

MINI_ERA = $(ESP_ROOT)/soft/ariane/mini-era

mini-era: soft-clean mini-era-distclean soft $(MINI_ERA_BUILD)/prom.srec $(MINI_ERA_BUILD)/ram.srec $(MINI_ERA_BUILD)/prom.bin $(MINI_ERA_BUILD)/mini_era.bin

mini-era-distclean: mini-era-clean

mini-era-clean:
	$(QUIET_CLEAN)$(RM)		 	\
		$(MINI_ERA_BUILD)/prom.srec 	\
		$(MINI_ERA_BUILD)/ram.srec		\
		$(MINI_ERA_BUILD)/prom.exe		\
		$(MINI_ERA_BUILD)/mini_era.exe	\
		$(MINI_ERA_BUILD)/prom.bin		\
		$(MINI_ERA_BUILD)/riscv.dtb		\
		$(MINI_ERA_BUILD)/startup.o		\
		$(MINI_ERA_BUILD)/main.o		\
		$(MINI_ERA_BUILD)/uart.o		\
		$(MINI_ERA_BUILD)/get_counter.o \
		$(MINI_ERA_BUILD)/read_trace.o \
		$(MINI_ERA_BUILD)/kernels_api.o \
		$(MINI_ERA_BUILD)/descrambler_function.o \
		$(MINI_ERA_BUILD)/viterbi_parms.o \
		$(MINI_ERA_BUILD)/viterbi_flat.o \
		$(MINI_ERA_BUILD)/mini_era.bin

$(MINI_ERA_BUILD)/riscv.dtb: $(ESP_CFG_BUILD)/riscv.dts $(ESP_CFG_BUILD)/socmap.vhd
	$(QUIET_BUILD) mkdir -p $(MINI_ERA_BUILD)
	@dtc -I dts $< -O dtb -o $@

$(MINI_ERA_BUILD)/startup.o: $(MINI_ERA)/src/startup.S $(MINI_ERA_BUILD)/riscv.dtb
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) cd $(MINI_ERA_BUILD); $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) -DSMP=$(SMP)\
		-c $< -o startup.o

$(MINI_ERA_BUILD)/main.o: $(BOOTROM_PATH)/main.c $(ESP_CFG_BUILD)/esplink.h
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(MINI_ERA_BUILD)/uart.o: $(BOOTROM_PATH)/uart.c $(ESP_CFG_BUILD)/esplink.h
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(MINI_ERA_BUILD)/prom.exe: $(MINI_ERA_BUILD)/startup.o $(MINI_ERA_BUILD)/uart.o $(MINI_ERA_BUILD)/main.o $(BOOTROM_PATH)/linker.lds
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-nostdlib -nodefaultlibs -nostartfiles \
		-T$(BOOTROM_PATH)/linker.lds \
		$(MINI_ERA_BUILD)/startup.o $(MINI_ERA_BUILD)/uart.o $(MINI_ERA_BUILD)/main.o \
		-o $@

$(MINI_ERA_BUILD)/prom.srec: $(MINI_ERA_BUILD)/prom.exe
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_OBJCP)$(CROSS_COMPILE_ELF)objcopy -O srec $< $@
	@cp $(MINI_ERA_BUILD)/prom.srec $(SOFT_BUILD)/prom.srec

$(MINI_ERA_BUILD)/prom.bin: $(MINI_ERA_BUILD)/prom.exe
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_OBJCP) $(CROSS_COMPILE_ELF)objcopy -O binary $< $@

RISCV_CFLAGS  = -I$(RISCV_TESTS)/env
RISCV_CFLAGS += -I$(RISCV_TESTS)/benchmarks/common
RISCV_CFLAGS += -I$(BOOTROM_PATH)
RISCV_CFLAGS += -mcmodel=medany
RISCV_CFLAGS += -static
RISCV_CFLAGS += -std=gnu99
RISCV_CFLAGS += -O2
RISCV_CFLAGS += -ffast-math
RISCV_CFLAGS += -fno-common
RISCV_CFLAGS += -fno-builtin-printf
RISCV_CFLAGS += -nostdlib
RISCV_CFLAGS += -nostartfiles -lm -lgcc

ADDN_RISCV_CFLAGS += -I$(RISCV_PK)/machine
ADDN_RISCV_CFLAGS += -I$(DESIGN_PATH)/$(ESP_CFG_BUILD)
ADDN_RISCV_CFLAGS += -I$(DRIVERS)/include/
ADDN_RISCV_CFLAGS += -I$(DRIVERS)/../common/include/
ADDN_RISCV_CFLAGS += -I$(DRIVERS)/common/include/
ADDN_RISCV_CFLAGS += -I$(DRIVERS)/baremetal/include/
ADDN_RISCV_CFLAGS += -I$(MINI_ERA)/include 

SPX_CFLAGS += -DHW_FFT -DUSE_FFT_FX=32 -DUSE_FFT_ACCEL_TYPE=1 -DFFT_SPANDEX_MODE=1 -DHW_FFT_BITREV
SPX_CFLAGS += -DHW_VIT -DVIT_DEVICE_NUM=0 -DVIT_SPANDEX_MODE=1
SPX_CFLAGS += -DDOUBLE_WORD
SPX_CFLAGS += -DUSE_ESP_INTERFACE -DITERATIONS=1000
SPX_CFLAGS += -DTWO_CORE_SCHED
SPX_CFLAGS += -DUSE_FFT_SENSOR
SPX_CFLAGS += -DUSE_VIT_SENSOR

$(MINI_ERA_BUILD)/get_counter.o: $(MINI_ERA)/src/get_counter.c
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-O2 \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(MINI_ERA_BUILD)/descrambler_function.o: $(MINI_ERA)/src/descrambler_function.c
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-O2 \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(MINI_ERA_BUILD)/viterbi_flat.o: $(MINI_ERA)/src/viterbi_flat.c
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc $(RISCV_CFLAGS) $(ADDN_RISCV_CFLAGS) \
		-O2 \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@


$(MINI_ERA_BUILD)/viterbi_parms.o: $(MINI_ERA)/src/viterbi_parms.c
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-O2 \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

OBJ =	$(MINI_ERA_BUILD)/get_counter.o \
		$(MINI_ERA_BUILD)/descrambler_function.o \
		$(MINI_ERA_BUILD)/viterbi_parms.o \
		$(MINI_ERA_BUILD)/viterbi_flat.o

$(MINI_ERA_BUILD)/mini_era.exe: $(MINI_ERA)/src/read_trace.c $(MINI_ERA)/src/calculate_dist_from_fmcw.c $(MINI_ERA)/src/fft.c $(MINI_ERA)/src/kernels_api.c $(MINI_ERA)/src/mini_main.c $(OBJ) $(MINI_ERA_BUILD)/uart.o
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc $(RISCV_CFLAGS) $(ADDN_RISCV_CFLAGS) $(SPX_CFLAGS) \
	$(MINI_ERA)/src/crt.S  \
	$(SOFT)/common/syscalls.c \
	-T $(RISCV_TESTS)/benchmarks/common/test.ld -o $@ \
	$(OBJ) \
	$(MINI_ERA_BUILD)/uart.o $(MINI_ERA)/src/read_trace.c $(MINI_ERA)/src/calculate_dist_from_fmcw.c $(MINI_ERA)/src/fft.c $(MINI_ERA)/src/kernels_api.c $(MINI_ERA)/src/mini_main.c \
	$(SOFT_BUILD)/drivers/probe/libprobe.a \
	$(SOFT_BUILD)/drivers/utils/baremetal/libutils.a

$(MINI_ERA_BUILD)/mini_era.bin: $(MINI_ERA_BUILD)/mini_era.exe
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -O binary $(MINI_ERA_BUILD)/mini_era.exe $@
	
$(MINI_ERA_BUILD)/ram.srec: $(MINI_ERA_BUILD)/mini_era.exe
	@mkdir -p $(MINI_ERA_BUILD)
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -O srec --gap-fill 0 $< $@
	@cp $(MINI_ERA_BUILD)/ram.srec $(SOFT_BUILD)/ram.srec
