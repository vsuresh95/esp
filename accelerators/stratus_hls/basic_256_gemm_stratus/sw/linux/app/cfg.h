// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "basic_256_gemm_stratus.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define C_MAT_OFFSET 0
#define B_MAT_OFFSET 0
#define A_MAT_OFFSET 0
#define K 1
#define M 1
#define N 1

/* <<--params-->> */
const int32_t C_mat_offset = C_MAT_OFFSET;
const int32_t B_mat_offset = B_MAT_OFFSET;
const int32_t A_mat_offset = A_MAT_OFFSET;
const int32_t K = K;
const int32_t M = M;
const int32_t N = N;

#define NACC 1

struct basic_256_gemm_stratus_access basic_256_gemm_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.C_mat_offset = C_MAT_OFFSET,
		.B_mat_offset = B_MAT_OFFSET,
		.A_mat_offset = A_MAT_OFFSET,
		.K = K,
		.M = M,
		.N = N,
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
		.devname = "basic_256_gemm_stratus.0",
		.ioctl_req = BASIC_256_GEMM_STRATUS_IOC_ACCESS,
		.esp_desc = &(basic_256_gemm_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
