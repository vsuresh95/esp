# Copyright (c) 2011-2021 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := multicore_random

NUM_THREADS ?= 2
TEST_ID ?= 0

EXTRA_CFLAGS += -DNUM_THREADS=$(NUM_THREADS)
EXTRA_CFLAGS += -DTEST_ID=$(TEST_ID)

CFLAGS += -I.

include $(DRIVERS)/common_bare.mk
