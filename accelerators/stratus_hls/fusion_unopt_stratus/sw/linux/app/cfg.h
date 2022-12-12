// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "fusion_unopt_vivado.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define VENO 10
#define IMGWIDTH 640
#define HTDIM 512
#define IMGHEIGHT 480
#define SDF_BLOCK_SIZE 8
#define SDF_BLOCK_SIZE3 512

/* <<--params-->> */
const int32_t veno = VENO;
const int32_t imgwidth = IMGWIDTH;
const int32_t htdim = HTDIM;
const int32_t imgheight = IMGHEIGHT;
const int32_t sdf_block_size = SDF_BLOCK_SIZE;
const int32_t sdf_block_size3 = SDF_BLOCK_SIZE3;

#define NACC 1

struct fusion_unopt_vivado_access fusion_unopt_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.veno = VENO,
		.imgwidth = IMGWIDTH,
		.htdim = HTDIM,
		.imgheight = IMGHEIGHT,
		.sdf_block_size = SDF_BLOCK_SIZE,
		.sdf_block_size3 = SDF_BLOCK_SIZE3,
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
		.devname = "fusion_unopt_vivado.0",
		.ioctl_req = FUSION_UNOPT_VIVADO_IOC_ACCESS,
		.esp_desc = &(fusion_unopt_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
