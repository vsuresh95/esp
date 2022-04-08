# Copyright (c) 2011-2021 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0


ARIANE ?= $(ESP_ROOT)/rtl/cores/ariane/ariane

RISCV_TESTS = $(SOFT)/riscv-tests
RISCV_PK = $(SOFT)/riscv-pk

soft: $(SOFT_BUILD)/prom.srec $(SOFT_BUILD)/ram.srec $(SOFT_BUILD)/prom.bin $(SOFT_BUILD)/systest.bin

MINI_ERA = $(ESP_ROOT)/soft/ariane/mini-era

soft-clean:
	$(QUIET_CLEAN)$(RM)		 	\
		$(SOFT_BUILD)/prom.srec 	\
		$(SOFT_BUILD)/ram.srec		\
		$(SOFT_BUILD)/prom.exe		\
		$(SOFT_BUILD)/systest.exe	\
		$(SOFT_BUILD)/prom.bin		\
		$(SOFT_BUILD)/riscv.dtb		\
		$(SOFT_BUILD)/startup.o		\
		$(SOFT_BUILD)/main.o		\
		$(SOFT_BUILD)/uart.o		\
		$(SOFT_BUILD)/get_counter.o \
		$(SOFT_BUILD)/read_trace.o \
		$(SOFT_BUILD)/kernels_api.o \
		$(SOFT_BUILD)/descrambler_function.o \
		$(SOFT_BUILD)/viterbi_parms.o \
		$(SOFT_BUILD)/viterbi_flat.o \
		$(SOFT_BUILD)/systest.bin

soft-distclean: soft-clean

$(SOFT_BUILD)/riscv.dtb: $(ESP_CFG_BUILD)/riscv.dts $(ESP_CFG_BUILD)/socmap.vhd
	$(QUIET_BUILD) mkdir -p $(SOFT_BUILD)
	@dtc -I dts $< -O dtb -o $@

$(SOFT_BUILD)/startup.o: $(BOOTROM_PATH)/startup.S $(SOFT_BUILD)/riscv.dtb
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) cd $(SOFT_BUILD); $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-c $< -o startup.o

$(SOFT_BUILD)/main.o: $(BOOTROM_PATH)/main.c $(ESP_CFG_BUILD)/socmap.h
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(SOFT_BUILD)/uart.o: $(BOOTROM_PATH)/uart.c $(ESP_CFG_BUILD)/socmap.h
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(SOFT_BUILD)/prom.exe: $(SOFT_BUILD)/startup.o $(SOFT_BUILD)/uart.o $(SOFT_BUILD)/main.o $(BOOTROM_PATH)/linker.lds
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-Os \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-nostdlib -nodefaultlibs -nostartfiles \
		-T$(BOOTROM_PATH)/linker.lds \
		$(SOFT_BUILD)/startup.o $(SOFT_BUILD)/uart.o $(SOFT_BUILD)/main.o \
		-o $@

$(SOFT_BUILD)/prom.srec: $(SOFT_BUILD)/prom.exe
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_OBJCP)$(CROSS_COMPILE_ELF)objcopy -O srec $< $@

$(SOFT_BUILD)/prom.bin: $(SOFT_BUILD)/prom.exe
	@mkdir -p $(SOFT_BUILD)
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
RISCV_CFLAGS += -I$(RISCV_PK)/machine
RISCV_CFLAGS += -I$(DESIGN_PATH)/$(ESP_CFG_BUILD)
RISCV_CFLAGS += -I$(DRIVERS)/include/
RISCV_CFLAGS += -I$(DRIVERS)/../common/include/
RISCV_CFLAGS += -I$(DRIVERS)/common/include/
RISCV_CFLAGS += -I$(DRIVERS)/baremetal/include/
RISCV_CFLAGS += -I$(MINI_ERA)/include

SPX_CFLAGS += -DHW_FFT -DUSE_FFT_FX=32 -DUSE_FFT_ACCEL_TYPE=1 -DFFT_SPANDEX_MODE=4 -DHW_FFT_BITREV
SPX_CFLAGS += -DHW_VIT -DVIT_DEVICE_NUM=0 -DVIT_SPANDEX_MODE=4
SPX_CFLAGS += -DDOUBLE_WORD
SPX_CFLAGS += -DUSE_ESP_INTERFACE -DITERATIONS=1000
SPX_CFLAGS += -DTWO_CORE_SCHED
SPX_CFLAGS += -DUSE_FFT_SENSOR
SPX_CFLAGS += -DUSE_VIT_SENSOR

$(SOFT_BUILD)/get_counter.o: $(MINI_ERA)/src/get_counter.c
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-O2 \
		-Wall -Werror \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

# $(SOFT_BUILD)/calculate_dist_from_fmcw.o: $(MINI_ERA)/src/calculate_dist_from_fmcw.c
# 	@mkdir -p $(SOFT_BUILD)
# 	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
# 		-O2 \
# 		-Wall -Werror \
# 		-mcmodel=medany -mexplicit-relocs \
# 		-I$(MINI_ERA)/include \
# 		-I$(BOOTROM_PATH) \
# 		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
# 		-c $< -o $@
# 
# $(SOFT_BUILD)/fft.o: $(MINI_ERA)/src/fft.c
# 	@mkdir -p $(SOFT_BUILD)
# 	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
# 		-O2 \
# 		-mcmodel=medany -mexplicit-relocs \
# 		-I$(MINI_ERA)/include \
# 		-I$(BOOTROM_PATH) \
# 		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
# 		-c $< -o $@

$(SOFT_BUILD)/descrambler_function.o: $(MINI_ERA)/src/descrambler_function.c
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-O2 \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@

$(SOFT_BUILD)/viterbi_flat.o: $(MINI_ERA)/src/viterbi_flat.c
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc $(RISCV_CFLAGS) \
		-O2 \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@


$(SOFT_BUILD)/viterbi_parms.o: $(MINI_ERA)/src/viterbi_parms.c
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
		-O2 \
		-mcmodel=medany -mexplicit-relocs $(SPX_CFLAGS) \
		-I$(MINI_ERA)/include \
		-I$(BOOTROM_PATH) \
		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
		-c $< -o $@


# $(SOFT_BUILD)/kernels_api.o: $(MINI_ERA)/src/kernels_api.c
# 	@mkdir -p $(SOFT_BUILD)
# 	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
# 		-O2 \
# 		-mcmodel=medany -mexplicit-relocs \
# 		-I$(MINI_ERA)/include \
# 		-I$(BOOTROM_PATH) \
# 		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
# 		-c $< -o $@
# 
# $(SOFT_BUILD)/read_trace.o: $(MINI_ERA)/src/read_trace.c
# 	@mkdir -p $(SOFT_BUILD)
# 	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc \
# 		-O2 \
# 		-mcmodel=medany -mexplicit-relocs \
# 		-I$(MINI_ERA)/include \
# 		-I$(BOOTROM_PATH) \
# 		-I$(DESIGN_PATH)/$(ESP_CFG_BUILD) \
# 		-c $< -o $@
# 
OBJ =	$(SOFT_BUILD)/get_counter.o \
		$(SOFT_BUILD)/descrambler_function.o \
		$(SOFT_BUILD)/viterbi_parms.o \
		$(SOFT_BUILD)/viterbi_flat.o
		#$(SOFT_BUILD)/fft.o \
		#$(SOFT_BUILD)/calculate_dist_from_fmcw.o \

$(SOFT_BUILD)/systest.exe: $(MINI_ERA)/src/read_trace.c $(MINI_ERA)/src/calculate_dist_from_fmcw.c $(MINI_ERA)/src/fft.c $(MINI_ERA)/src/kernels_api.c $(MINI_ERA)/src/mini_main.c $(OBJ) $(SOFT_BUILD)/uart.o
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CC) $(CROSS_COMPILE_ELF)gcc $(RISCV_CFLAGS) $(SPX_CFLAGS) \
	$(RISCV_TESTS)/benchmarks/common/crt.S  \
	$(SOFT)/common/syscalls.c \
	-T $(RISCV_TESTS)/benchmarks/common/test.ld -o $@ \
	$(OBJ) \
	$(SOFT_BUILD)/uart.o $(MINI_ERA)/src/read_trace.c $(MINI_ERA)/src/calculate_dist_from_fmcw.c $(MINI_ERA)/src/fft.c $(MINI_ERA)/src/kernels_api.c $(MINI_ERA)/src/mini_main.c \
	$(SOFT_BUILD)/drivers/probe/libprobe.a \
	$(SOFT_BUILD)/drivers/utils/baremetal/libutils.a

$(SOFT_BUILD)/systest.bin: $(TEST_PROGRAM)
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -O binary $< $@

$(SOFT_BUILD)/ram.srec: $(TEST_PROGRAM)
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -O srec --gap-fill 0 $< $@


$(SOFT_BUILD)/sysroot:
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_CP)cp -r $(SOFT)/sysroot $(SOFT_BUILD)

$(SOFT_BUILD)/sysroot.files: $(SOFT_BUILD)/sysroot
	@mkdir -p $(SOFT_BUILD)
	$(QUIET_MAKE)$(MAKE) -C ${LINUXSRC}/usr gen_init_cpio
	$(QUIET_INFO)echo "Generating root file-system list..."
	@sh ${LINUXSRC}/usr/gen_initramfs_list.sh -u `id -u` -g `id -g` $< \
	    | sed -e 's/^file \(\/bin\/busybox .*\) 755 0 0/file \1 4755 0 0/' \
	    > $@;
	@echo "nod /dev/console 622 0 0 c 5 1" >> $@
	@touch $@


$(SOFT_BUILD)/sysroot.cpio: $(SOFT_BUILD)/sysroot.files
	$(QUIET_BUILD)${LINUXSRC}/usr/gen_init_cpio $< > $@


$(SOFT_BUILD)/linux-build/.config: $(LINUXSRC)/arch/$(ARCH)/configs/$(LINUX_CONFIG)
	@$(MAKE) $(SOFT_BUILD)/linux-build
	$(QUIET_MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_LINUX) $(MAKE)  O=$(SOFT_BUILD)/linux-build -C ${LINUXSRC} $(LINUX_CONFIG)


$(SOFT_BUILD)/linux-build/vmlinux: $(SOFT_BUILD)/sysroot.cpio $(SOFT_BUILD)/linux-build/.config
	$(QUIET_MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_LINUX) $(MAKE) -C $(SOFT_BUILD)/linux-build


$(SOFT_BUILD)/pk-build:
	$(QUIET_MKDIR)mkdir -p $@


$(SOFT_BUILD)/pk-build/bbl: $(SOFT_BUILD)/pk-build sysroot-update
	$(QUIET_CHECK) cd $(SOFT_BUILD)/pk-build; \
		if ! test -e Makefile; then \
			$(RISCV_PK)/configure \
				--host=riscv64-unknown-elf \
				CC=$(CROSS_COMPILE_ELF)gcc \
				OBJDUMP=riscv64-unknown-elf-objdump \
				--with-payload=../linux-build/vmlinux; \
		fi;
	$(QUIET_MAKE) $(MAKE) -C $(SOFT_BUILD)/pk-build


$(SOFT_BUILD)/linux.bin: $(SOFT_BUILD)/pk-build/bbl
	$(QUIET_OBJCP) riscv64-unknown-elf-objcopy -S -O binary --change-addresses -0x80000000 $< $@


linux: $(SOFT_BUILD)/linux.bin $(SOFT_BUILD)/prom.bin


linux-clean: sysroot-clean
	$(QUIET_CLEAN)
	@if test -e $(SOFT_BUILD)/linux-build; then \
		ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE_LINUX) $(MAKE) --quiet -C $(SOFT_BUILD)/linux-build clean; \
	fi;


linux-distclean: sysroot-distclean
	$(QUIET_CLEAN)$(RM) $(SOFT_BUILD)/linux-build $(SOFT_BUILD)/pk-build $(SOFT_BUILD)/linux.bin


### Flags

## Genus
GENUS_VLOGOPT += -define WT_DCACHE=1

## Modelsim
VLOGOPT +=
VLOGOPT += -incr
VLOGOPT += -64
VLOGOPT += -nologo
VLOGOPT += -suppress 13262
VLOGOPT += -suppress 2286
VLOGOPT += -permissive
VLOGOPT += +define+WT_DCACHE
VLOGOPT += -pedanticerrors
VLOGOPT += -suppress 2583
ifeq ("$(CPU_ARCH)", "ariane")
VSIMOPT += +UVM_NO_RELNOTES -64 +permissive-off
VSIMOPT += -voptargs="+acc"
else
VSIMOPT += -novopt
endif

## Xcelium
XMLOGOPT +=
# Define verilator env because Xcelium do not support SVAs and UVM in Ariane
XMLOGOPT += -DEFINE VERILATOR
XMLOGOPT += -UNCLOCKEDSVA
XMLOGOPT += -DEFINE WT_DCACHE=1


### Incdir and RTL

ifeq ("$(CPU_ARCH)", "ariane")
INCDIR += $(ARIANE)/src/common_cells/include
VERILOG_ARIANE += $(foreach f, $(shell strings $(FLISTS)/ariane_vlog.flist), $(ARIANE)/$(f))
VERILOG_ARIANE += $(DESIGN_PATH)/$(ESP_CFG_BUILD)/plic_regmap.sv
ifneq ($(filter $(TECHLIB),$(FPGALIBS)),)
VERILOG_ARIANE += $(foreach f, $(shell strings $(FLISTS)/ariane_fpga_vlog.flist), $(ARIANE)/$(f))
endif
THIRDPARTY_VLOG += $(VERILOG_ARIANE)
endif

