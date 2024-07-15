# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
EXTRA_CFLAGS ?=
APPNAME := sort

IS_ESP ?= 1
COH_MODE ?= 0
ITERATIONS ?= 100
SORT_LEN ?= 32
ENABLE_SM ?= 0

EXTRA_CFLAGS += -DIS_ESP=$(IS_ESP)
EXTRA_CFLAGS += -DCOH_MODE=$(COH_MODE)
EXTRA_CFLAGS += -DITERATIONS=$(ITERATIONS)
EXTRA_CFLAGS += -DENABLE_SM=$(ENABLE_SM)

include $(DRIVERS)/common.mk
