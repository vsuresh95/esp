// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "rotate_order_stratus.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define NUM_CHANNEL 16
#define BLOCK_SIZE 1024

/* <<--params-->> */
const int32_t num_channel = NUM_CHANNEL;
const int32_t block_size = BLOCK_SIZE;

#define NACC 1

struct rotate_order_stratus_access rotate_order_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.num_channel = NUM_CHANNEL,
		.block_size = BLOCK_SIZE,
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
		.devname = "rotate_order_stratus.0",
		.ioctl_req = ROTATE_ORDER_STRATUS_IOC_ACCESS,
		.esp_desc = &(rotate_order_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
