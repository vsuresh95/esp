# Copyright (c) 2011-2021 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
include ../driver.mk

CFLAGS := -Wall -Werror -O3 -I$(DRIVERS)/include -I$(DRIVERS)/../common/include -L.
KBUILD_EXTRA_SYMBOLS := ../drivers/esp_cache/Module.symvers
KBUILD_EXTRA_SYMBOLS += ../drivers/contig_alloc/Module.symvers
export KBUILD_EXTRA_SYMBOLS

Module.symvers:
	$(MAKE) -C ../esp_cache ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) DRIVERS=$(DRIVERS)
	$(MAKE) -C ../contig_alloc ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) DRIVERS=$(DRIVERS)
#	cat ../esp_cache/$@ > $@
#	cat ../contig_alloc/$@ >> $@
