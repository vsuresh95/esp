# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := audio_fft

COH_MODE ?= 0
IS_ESP ?= 1
ITERATIONS ?= 100
LOG_LEN ?= 10
ENABLE_SM ?= 0

EXTRA_CFLAGS += -DCOH_MODE=$(COH_MODE)
EXTRA_CFLAGS += -DIS_ESP=$(IS_ESP)
EXTRA_CFLAGS += -DITERATIONS=$(ITERATIONS)
EXTRA_CFLAGS += -DLOG_LEN=$(LOG_LEN)
EXTRA_CFLAGS += -DENABLE_SM=$(ENABLE_SM)

include $(DRIVERS)/common_bare.mk
