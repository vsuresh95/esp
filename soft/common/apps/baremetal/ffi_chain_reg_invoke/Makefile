# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := ffi_chain_reg_invoke

COH_MODE ?= 0
IS_ESP ?= 1
ITERATIONS ?= 1000
LOG_LEN ?= 10
SW_FIR ?= 0

EXTRA_CFLAGS += -DCOH_MODE=$(COH_MODE)
EXTRA_CFLAGS += -DIS_ESP=$(IS_ESP)
EXTRA_CFLAGS += -DITERATIONS=$(ITERATIONS)
EXTRA_CFLAGS += -DLOG_LEN=$(LOG_LEN)
EXTRA_CFLAGS += -DSW_FIR=$(SW_FIR)

include $(DRIVERS)/common_bare.mk
