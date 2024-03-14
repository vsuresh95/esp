# Copyright (c) 2011-2021 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := sensor_dma

COH_MODE ?= 0
IS_ESP ?= 1
ITERATIONS ?= 1000
LOG_LEN ?= 11

EXTRA_CFLAGS += -DCOH_MODE=$(COH_MODE)
EXTRA_CFLAGS += -DIS_ESP=$(IS_ESP)
EXTRA_CFLAGS += -DITERATIONS=$(ITERATIONS)
EXTRA_CFLAGS += -DLOG_LEN=$(LOG_LEN)

include $(DRIVERS)/common_bare.mk