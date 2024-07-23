// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "gemm_stratus.h"

#define MAX_DEVICES 3
// #define NUM_DEVICES 3
#define ITERATIONS 1000

// Comp Mode 0: Reg Inv
// Comp Mode 1: Chaining
// Comp Mode 2: Pipelining
#define MODE_REG_INV 0
#define MODE_CHAIN 1
#define MODE_PIPE 2

#define SYNC_VAR_SIZE 6
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define END_FLAG_OFFSET 2
#define READY_FLAG_OFFSET 4

#define PROD_VALID_OFFSET 0x404
#define PROD_READY_OFFSET 0x400
#define CONS_VALID_OFFSET 0x4
#define CONS_READY_OFFSET 0x0
#define INPUT_OFFSET 0x8
#define OUTPUT_OFFSET 0x408

/* User defined */

// Define data type (decomment the one needed)
// #define __UINT
// #define __INT
#define __FIXED
// #define __FLOAT

// Define bit width (decomment the one needed)
// #ifndef __riscv
#define BITWIDTH 32
// #define BITWIDTH 64
// #else
// #define BITWIDTH 32
// #define BITWIDTH 64
// #endif

/* End of user defined */

#ifdef __UINT
#if (BITWIDTH == 32)
typedef unsigned token_t;
#elif (BITWIDTH == 64)
typedef long long unsigned token_t;
#endif
#endif

#ifdef __INT
#if (BITWIDTH == 32)
typedef int token_t;
#elif (BITWIDTH == 64)
typedef long long token_t;
#endif
#endif

#ifdef __FIXED
#if (BITWIDTH == 32)
typedef int token_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 16
#elif (BITWIDTH == 64)
typedef long long token_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 32
#endif
#endif

#ifdef __FLOAT
#if (BITWIDTH == 32)
typedef float token_t;
#elif (BITWIDTH == 64)
typedef double token_t;
#endif
#endif

typedef float native_t;

#define MAX_PRINTED_ERRORS 512

#define NACC 1
#define ACC_TLB_ENTRIES 128
#define ACC_PAGE_SIZE (1 << 20)
#define MAX_SIZE (ACC_PAGE_SIZE * ACC_TLB_ENTRIES)
#define MAX_TESTS 30


#ifdef FCN

struct gemm_stratus_access gemm_cfg_000[MAX_DEVICES]; 
struct esp_access esp ={
	.coherence = ACC_COH_FULL,
	.p2p_store = 0,
	.p2p_nsrcs = 0,
	.p2p_srcs = {"", "", "", ""},
	.start_stop = 1,
};

esp_thread_info_t cfg_000[MAX_DEVICES];
// esp_thread_info_t cfg_000[] = {
// 	{
// 		.run = true,
// 		.devname = "gemm_stratus.0",
// 		.ioctl_req = GEMM_STRATUS_IOC_ACCESS,
// 		.esp_desc = &(gemm_cfg_000[0].esp),
// 	}
// };

int mode = COMP_MODE;


#else

/* <<--params-def-->> */
#define T 0

#define DO_RELU 0
#define TRANSPOSE 1
#define NINPUTS 1
#define D3 D
#define D2 D
#define D1 D
#define ST_OFFSET (NINPUTS * (D1 * D2 + D2 * D3))
#define LD_OFFSET1 0
#define LD_OFFSET2 (NINPUTS * (D1 * D2))
#define D 16

struct gemm_stratus_access gemm_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.do_relu = DO_RELU,
		.transpose = TRANSPOSE,
		.ninputs = NINPUTS,
		.d3 = D3,
		.d2 = D2,
		.d1 = D1,
		.st_offset = ST_OFFSET,
		.ld_offset1 = LD_OFFSET1,
		.ld_offset2 = LD_OFFSET2,
		.prod_valid_offset = CONS_VALID_OFFSET,
		.prod_ready_offset = CONS_READY_OFFSET,
		.cons_valid_offset = PROD_VALID_OFFSET,
		.cons_ready_offset = PROD_READY_OFFSET,
		// .input_offset = 0,
        // .output_offset = 0,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_FULL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

// ASI-enabled thread
esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "gemm_stratus.2",
		.ioctl_req = GEMM_STRATUS_IOC_ACCESS,
		.esp_desc = &(gemm_cfg_000[0].esp),
	}
};


// Linux-invoc thread
esp_thread_info_t cfg_001[] = {
{
	.run = true,
	.devname = "gemm_stratus.3",
	.ioctl_req = GEMM_STRATUS_IOC_ACCESS,
	.esp_desc = &(gemm_cfg_000[0].esp),
}
};

#endif
#endif /* __ESP_CFG_000_H__ */
