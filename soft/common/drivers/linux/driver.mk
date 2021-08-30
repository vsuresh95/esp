# Copyright (c) 2011-2021 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
ifeq ("$(CPU_ARCH)", "ariane")
CROSS_COMPILE ?= riscv64-unknown-linux-gnu-
ARCH ?= riscv
else # ("$(CPU_ARCH)", "leon3")
CROSS_COMPILE ?= sparc-linux-
ARCH ?= sparc
endif

ESP_CORE_PATH ?= $(DRIVERS)/esp
KBUILD_EXTRA_SYMBOLS := ../drivers/esp/Module.symvers
export KBUILD_EXTRA_SYMBOLS

all: check Module.symvers
	make -C $(KSRC) M=`pwd` CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH)

check:
ifeq ($(KSRC),)
	$(error 'Path to kernel in env variable KSRC not found. Exiting')
endif
.PHONY: check

clean: check
	rm -rf $(ESP_CORE_PATH)

help: check
	$(MAKE) -C $(KSRC) M=`pwd` CROSS_COMPILE=$(CROSS_COMPILE) ARCH=$(ARCH) help

.PHONY: all clean help
