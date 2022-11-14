// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "isca_synth_stratus.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define COMPUTE_RATIO 1
#define SIZE 1

/* <<--params-->> */
int32_t compute_ratio = 40;
int32_t size = 128;
int32_t speedup = 20;

#define NACC 1

#define ITERATIONS 20

struct isca_synth_stratus_access isca_synth_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.compute_ratio = COMPUTE_RATIO,
		.size = SIZE,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_FULL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "isca_synth_stratus.0",
		.ioctl_req = ISCA_SYNTH_STRATUS_IOC_ACCESS,
		.esp_desc = &(isca_synth_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
