// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "audio_fft_stratus.h"

typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 14

/* <<--params-def-->> */
#define DO_INVERSE 0
#define DO_SHIFT 0

/* <<--params-->> */
const int32_t logn_samples = LOG_LEN;
const int32_t do_inverse = DO_INVERSE;
const int32_t do_shift = DO_SHIFT;


#define NACC 1

struct audio_fft_stratus_access audio_fft_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_inverse = 0,
		.logn_samples = LOG_LEN,
		.do_shift = DO_SHIFT,

		.prod_valid_offset = 0,
		.prod_ready_offset = 0,
		.cons_valid_offset = 0,
		.cons_ready_offset = 0,
		.input_offset = 0,
		.output_offset = 0,
		
		.src_offset = 0,
		.dst_offset = 0,
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "audio_fft_stratus.0",
		.ioctl_req = AUDIO_FFT_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_fft_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
