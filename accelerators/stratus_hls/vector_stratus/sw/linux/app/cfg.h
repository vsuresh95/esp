// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "vector_stratus.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define LEN 1
#define OUTPUT_OFFSET 1
#define INPUT1_OFFSET 1
#define INPUT2_OFFSET 1

/* <<--params-->> */
const int32_t input_len = LEN;
const int32_t output_offset = OUTPUT_OFFSET;
const int32_t input1_offset = INPUT1_OFFSET;
const int32_t input2_offset = INPUT2_OFFSET;

#define NACC 1

struct vector_stratus_access vector_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.input_len = LEN,
		.output_offset = OUTPUT_OFFSET,
		.input1_offset = INPUT1_OFFSET,
		.input2_offset = INPUT2_OFFSET,
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
		.devname = "vector_stratus.0",
		.ioctl_req = VECTOR_STRATUS_IOC_ACCESS,
		.esp_desc = &(vector_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
