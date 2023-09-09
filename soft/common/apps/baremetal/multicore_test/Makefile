# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := multicore_test

NUM_THREADS ?= 2
EXTRA_CFLAGS += -DNUM_THREADS=$(NUM_THREADS)

CFLAGS += -I.

include $(DRIVERS)/common_bare.mk
