// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "mesh_gen_stratus.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define NUM_HASH_TABLE 1

/* <<--params-->> */
const int32_t num_hash_table = NUM_HASH_TABLE;

#define NACC 1

struct mesh_gen_stratus_access mesh_gen_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.num_hash_table = NUM_HASH_TABLE,
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
		.devname = "mesh_gen_stratus.0",
		.ioctl_req = MESH_GEN_STRATUS_IOC_ACCESS,
		.esp_desc = &(mesh_gen_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
