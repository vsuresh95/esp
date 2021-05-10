// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "gemm_2_block_stratus.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define GEMM_M 64
#define GEMM_N 64
#define GEMM_K 64
#define GEMM_BATCH 1

/* <<--params-->> */
const int32_t gemm_m = GEMM_M;
const int32_t gemm_n = GEMM_N;
const int32_t gemm_k = GEMM_K;
const int32_t gemm_batch = GEMM_BATCH;

#define NACC 1

struct gemm_2_block_stratus_access gemm_2_block_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.gemm_m = GEMM_M,
		.gemm_n = GEMM_N,
		.gemm_k = GEMM_K,
		.gemm_batch = GEMM_BATCH,
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
		.devname = "gemm_2_block_stratus.0",
		.ioctl_req = GEMM_2_BLOCK_STRATUS_IOC_ACCESS,
		.esp_desc = &(gemm_2_block_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
