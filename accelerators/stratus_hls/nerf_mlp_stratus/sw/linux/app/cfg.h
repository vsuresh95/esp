// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "nerf_mlp_stratus.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define BATCH_SIZE 1

/* <<--params-->> */
const int32_t batch_size = BATCH_SIZE;

#define NACC 1

struct nerf_mlp_stratus_access nerf_mlp_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.batch_size = BATCH_SIZE,
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
		.devname = "nerf_mlp_stratus.0",
		.ioctl_req = NERF_MLP_STRATUS_IOC_ACCESS,
		.esp_desc = &(nerf_mlp_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
