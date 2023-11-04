# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
APPNAME := ffi_chain_sm_invoke

EXTRA_CFLAGS += -I$(ESP_ROOT)/accelerators/stratus_hls/audio_fft_stratus/sw/linux/include
EXTRA_CFLAGS += -I$(ESP_ROOT)/accelerators/stratus_hls/audio_fir_stratus/sw/linux/include

COH_MODE ?= 0
IS_ESP ?= 1
ITERATIONS ?= 1000
LOG_LEN ?= 10
IS_PIPELINE ?= 0

EXTRA_CFLAGS += -DCOH_MODE=$(COH_MODE)
EXTRA_CFLAGS += -DIS_ESP=$(IS_ESP)
EXTRA_CFLAGS += -DITERATIONS=$(ITERATIONS)
EXTRA_CFLAGS += -DLOG_LEN=$(LOG_LEN)
EXTRA_CFLAGS += -DIS_PIPELINE=$(IS_PIPELINE)

include $(DRIVERS)/common.mk
