# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

AUDIO_PIPELINE_BUILD = $(SOFT_BUILD)/audio_pipeline

ARIANE ?= $(ESP_ROOT)/rtl/cores/ariane/ariane

RISCV_TESTS = $(SOFT)/riscv-tests
RISCV_PK = $(SOFT)/riscv-pk
OPENSBI = $(SOFT)/opensbi

AUDIO_PIPELINE = $(ESP_ROOT)/soft/ariane/audio_pipeline

audio-pipeline: audio-pipeline-distclean $(AUDIO_PIPELINE_BUILD)/prom.srec $(AUDIO_PIPELINE_BUILD)/ram.srec $(AUDIO_PIPELINE_BUILD)/prom.bin $(AUDIO_PIPELINE_BUILD)/audio_pipeline.bin

audio-pipeline-distclean: audio-pipeline-clean

audio-pipeline-clean:
	$(QUIET_CLEAN)$(RM)		 	\
		$(AUDIO_PIPELINE_BUILD)/prom.srec 	\
		$(AUDIO_PIPELINE_BUILD)/ram.srec		\
		$(AUDIO_PIPELINE_BUILD)/prom.exe		\
		$(AUDIO_PIPELINE_BUILD)/audio_pipeline.exe	\
		$(AUDIO_PIPELINE_BUILD)/prom.bin		\
		$(AUDIO_PIPELINE_BUILD)/riscv.dtb		\
		$(AUDIO_PIPELINE_BUILD)/startup.o		\
		$(AUDIO_PIPELINE_BUILD)/main.o		\
		$(AUDIO_PIPELINE_BUILD)/uart.o		\
		$(AUDIO_PIPELINE_BUILD)/audio_pipeline.bin

$(AUDIO_PIPELINE_BUILD)/riscv.dtb: $(ESP_CFG_BUILD)/riscv.dts $(ESP_CFG_BUILD)/socmap.vhd
	$(QUIET_BUILD) mkdir -p $(AUDIO_PIPELINE_BUILD)
	@dtc -I dts $< -O dtb -o $@

$(AUDIO_PIPELINE_BUILD)/startup.o: $(BOOTROM_PATH)/startup.S $(AUDIO_PIPELINE_BUILD)/riscv.dtb
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_CC) cd $(AUDIO_PIPELINE_BUILD); $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) -DSMP=$(SMP)\
		-c $< -o startup.o

$(AUDIO_PIPELINE_BUILD)/main.o: $(BOOTROM_PATH)/main.c $(ESP_CFG_BUILD)/esplink.h
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(AUDIO_PIPELINE_BUILD)/uart.o: $(BOOTROM_PATH)/uart.c $(ESP_CFG_BUILD)/esplink.h
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(AUDIO_PIPELINE_BUILD)/prom.exe: $(AUDIO_PIPELINE_BUILD)/startup.o $(AUDIO_PIPELINE_BUILD)/uart.o $(AUDIO_PIPELINE_BUILD)/main.o $(BOOTROM_PATH)/linker.lds
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-nostdlib -nodefaultlibs -nostartfiles \
		-T$(BOOTROM_PATH)/linker.lds \
		$(AUDIO_PIPELINE_BUILD)/startup.o $(AUDIO_PIPELINE_BUILD)/uart.o $(AUDIO_PIPELINE_BUILD)/main.o \
		-o $@
	@cp $(AUDIO_PIPELINE_BUILD)/prom.exe $(SOFT_BUILD)/prom.exe

$(AUDIO_PIPELINE_BUILD)/prom.srec: $(AUDIO_PIPELINE_BUILD)/prom.exe
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_OBJCP)$(CROSS_COMPILE_ELF)objcopy -O srec $< $@
	@cp $(AUDIO_PIPELINE_BUILD)/prom.srec $(SOFT_BUILD)/prom.srec

$(AUDIO_PIPELINE_BUILD)/prom.bin: $(AUDIO_PIPELINE_BUILD)/prom.exe
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_OBJCP) $(CROSS_COMPILE_ELF)objcopy -O binary $< $@
	@cp $(AUDIO_PIPELINE_BUILD)/prom.bin $(SOFT_BUILD)/prom.bin

RISCV_CFLAGS  = -I$(RISCV_TESTS)/env
RISCV_CFLAGS += -I$(RISCV_TESTS)/benchmarks/common
RISCV_CFLAGS += -I$(BOOTROM_PATH)
RISCV_CFLAGS += -mcmodel=medany
RISCV_CFLAGS += -static
RISCV_CFLAGS += -O2
RISCV_CFLAGS += -ffast-math
RISCV_CFLAGS += -fno-common
RISCV_CFLAGS += -fno-builtin-printf
RISCV_CFLAGS += -nostdlib
RISCV_CFLAGS += -nostartfiles -lm -lgcc

RISCV_CFLAGS += -I$(RISCV_PK)/machine
RISCV_CFLAGS += -I$(DESIGN_PATH)/$(ESP_CFG_BUILD)
RISCV_CFLAGS += -I$(DRIVERS)/include/
RISCV_CFLAGS += -I$(DRIVERS)/../common/include/
RISCV_CFLAGS += -I$(DRIVERS)/common/include/
RISCV_CFLAGS += -I$(DRIVERS)/baremetal/include/
RISCV_CFLAGS += -I$(AUDIO_PIPELINE)/include 

NUM_BLOCKS ?= 16
BLOCK_SIZE ?= 1024
SAMPLERATE ?= 48000
NORDER ?= 3
NUM_SRCS ?= 16
COH_MODE ?= 0
IS_ESP ?= 1
DO_CHAIN_OFFLOAD ?= 0
DO_NP_CHAIN_OFFLOAD ?= 0
USE_INT ?= 1

RISCV_CFLAGS += -DNUM_BLOCKS=$(NUM_BLOCKS)
RISCV_CFLAGS += -DBLOCK_SIZE=$(BLOCK_SIZE)
RISCV_CFLAGS += -DSAMPLERATE=$(SAMPLERATE)
RISCV_CFLAGS += -DNORDER=$(NORDER)
RISCV_CFLAGS += -DNUM_SRCS=$(NUM_SRCS)
RISCV_CFLAGS += -DCOH_MODE=$(COH_MODE)
RISCV_CFLAGS += -DIS_ESP=$(IS_ESP)
RISCV_CFLAGS += -DDO_CHAIN_OFFLOAD=$(DO_CHAIN_OFFLOAD)
RISCV_CFLAGS += -DDO_NP_CHAIN_OFFLOAD=$(DO_NP_CHAIN_OFFLOAD)
RISCV_CFLAGS += -DUSE_INT=$(USE_INT)

RISCV_CPPFLAGS += $(RISCV_CFLAGS)
RISCV_CFLAGS += -std=gnu99

AUDIO_SRCS = \
	$(AUDIO_PIPELINE)/src/AudioBase.cpp \
	$(AUDIO_PIPELINE)/src/BFormat.cpp \
	$(AUDIO_PIPELINE)/src/AmbisonicBinauralizer.cpp \
	$(AUDIO_PIPELINE)/src/AmbisonicProcessor.cpp \
	$(AUDIO_PIPELINE)/src/AmbisonicZoomer.cpp \
	$(AUDIO_PIPELINE)/src/audio.cpp \
	$(AUDIO_PIPELINE)/src/audio_main.cpp \
	$(AUDIO_PIPELINE)/src/kiss_fft.cpp \
	$(AUDIO_PIPELINE)/src/kiss_fftr.cpp \
	$(AUDIO_PIPELINE)/src/FFTAcc.cpp \
	$(AUDIO_PIPELINE)/src/FIRAcc.cpp \
	$(AUDIO_PIPELINE)/src/FFIChain.cpp

$(AUDIO_PIPELINE_BUILD)/audio_pipeline.exe: $(AUDIO_SRCS) $(AUDIO_PIPELINE_BUILD)/uart.o
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc $(RISCV_CPPFLAGS) \
	$(RISCV_TESTS)/benchmarks/common/crt.S  \
	$(SOFT)/common/syscalls.c \
	-T $(RISCV_TESTS)/benchmarks/common/test.ld -o $@ \
	$(AUDIO_SRCS) \
	$(AUDIO_PIPELINE_BUILD)/uart.o \
	$(SOFT_BUILD)/drivers/probe/libprobe.a \
	$(SOFT_BUILD)/drivers/utils/baremetal/libutils.a

$(AUDIO_PIPELINE_BUILD)/audio_pipeline.bin: $(AUDIO_PIPELINE_BUILD)/audio_pipeline.exe
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -O binary $(AUDIO_PIPELINE_BUILD)/audio_pipeline.exe $@
	@cp $(AUDIO_PIPELINE_BUILD)/audio_pipeline.bin $(SOFT_BUILD)/systest.bin
	
$(AUDIO_PIPELINE_BUILD)/ram.srec: $(AUDIO_PIPELINE_BUILD)/audio_pipeline.exe
	@mkdir -p $(AUDIO_PIPELINE_BUILD)
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -O srec --gap-fill 0 $< $@
	@cp $(AUDIO_PIPELINE_BUILD)/ram.srec $(SOFT_BUILD)/ram.srec

fpga-run-audio-pipeline: esplink audio-pipeline
	@./$(ESP_CFG_BUILD)/esplink --reset
	@./$(ESP_CFG_BUILD)/esplink --brom -i $(SOFT_BUILD)/prom.bin
	@./$(ESP_CFG_BUILD)/esplink --dram -i $(SOFT_BUILD)/systest.bin
	@./$(ESP_CFG_BUILD)/esplink --reset

xmsim-compile-audio-pipeline: socketgen check_all_srcs audio-pipeline xcelium/xmready xcelium/xmsim.in
	$(QUIET_MAKE) \
	cd xcelium; \
	rm -f prom.srec ram.srec; \
	ln -s $(SOFT_BUILD)/prom.srec; \
	ln -s $(SOFT_BUILD)/ram.srec; \
	echo $(SPACES)"$(XMUPDATE) $(SIMTOP)"; \
	$(XMUPDATE) $(SIMTOP);

xmsim-audio-pipeline: xmsim-compile-audio-pipeline
	@cd xcelium; \
	echo $(SPACES)"$(XMSIM)"; \
	$(XMSIM); \
	cd ../

xmsim-gui-audio-pipeline: xmsim-compile-audio-pipeline
	@cd xcelium; \
	echo $(SPACES)"$(XMSIM) -gui"; \
	$(XMSIM) -gui; \
	cd ../
