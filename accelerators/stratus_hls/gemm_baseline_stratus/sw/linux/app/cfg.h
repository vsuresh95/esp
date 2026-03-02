// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "gemm_stratus.h"
#include <sys/epoll.h>

// Size and parameter defines
#define VALID_OFFSET 0
#define PAYLOAD_OFFSET 8
#define SM_INFO_SIZE 8

typedef int token_t;
typedef float native_t;
#define FX_IL 16

unsigned ITERATIONS = 100;

/* <<--params-def-->> */
unsigned dim_m = 16;
unsigned dim_n = 16;
unsigned dim_k = 16;

// ESP API for getting contig_alloc handle
extern contig_handle_t *lookup_handle(void *buf, enum contig_alloc_policy *policy);

uint64_t get_counter() {
    uint64_t t;
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t)
		:
		: "t0"
	);

	return t;
}

uint64_t t_sw;
uint64_t t_acc;

#endif /* __ESP_CFG_000_H__ */
