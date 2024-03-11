// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "audio_dma_stratus.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define SIZE 1

/* <<--params-->> */
const int32_t size = SIZE;

#define NACC 1

struct audio_dma_stratus_access audio_dma_cfg_000[] = {
	{
		/* <<--descriptor-->> */
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
		.devname = "audio_dma_stratus.0",
		.ioctl_req = SM_SENSOR_STRATUS_IOC_ACCESS,
		.esp_desc = &(audio_dma_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
