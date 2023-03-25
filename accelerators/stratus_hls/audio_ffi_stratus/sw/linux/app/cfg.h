// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "audio_ffi_stratus.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define DO_INVERSE 1
#define LOGN_SAMPLES 11
#define DO_SHIFT 1

/* <<--params-->> */
const int32_t do_inverse = DO_INVERSE;
const int32_t logn_samples = LOGN_SAMPLES;
const int32_t do_shift = DO_SHIFT;

#define NACC 1

struct audio_ffi_stratus_access audio_ffi_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_inverse = DO_INVERSE,
		.logn_samples = LOGN_SAMPLES,
		.do_shift = DO_SHIFT,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_NONE,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "audio_ffi_stratus.0",
		.ioctl_req = AUDIO_FFI_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_ffi_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
