# Copyright (c) 2011-2023 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := gemm

COH_MODE ?= 0
COMP_MODE ?= 1
NUM_DEVICES ?= 1
D3 ?= 16
D2 ?= 16
D1 ?= 16


ifneq ("$(SPX)", "")
        EXTRA_CFLAGS+=-DSPX=1
        EXTRA_CFLAGS+=-DIS_ESP=0
else
        EXTRA_CFLAGS+=-DIS_ESP=1
endif

ifneq ("$(FCN)", "")
        EXTRA_CFLAGS+=-DFCN=1
endif

EXTRA_CFLAGS += -DCOH_MODE=$(COH_MODE)
EXTRA_CFLAGS += -DCOMP_MODE=$(COMP_MODE)
EXTRA_CFLAGS += -DNUM_DEVICES=$(NUM_DEVICES)


include $(DRIVERS)/common_bare.mk
