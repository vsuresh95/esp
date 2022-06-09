// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "sensor_dma_tiled_stratus.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define SIZE 1

/* <<--params-->> */
const int32_t size = SIZE;

#define NACC 1

struct sensor_dma_tiled_stratus_access sensor_dma_tiled_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.size = SIZE,
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
		.devname = "sensor_dma_tiled_stratus.0",
		.ioctl_req = SENSOR_DMA_TILED_STRATUS_IOC_ACCESS,
		.esp_desc = &(sensor_dma_tiled_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
