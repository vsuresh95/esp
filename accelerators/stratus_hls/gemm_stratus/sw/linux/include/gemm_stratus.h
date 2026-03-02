// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _GEMM_STRATUS_H_
#define _GEMM_STRATUS_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#include <esp.h>
#include <esp_accelerator.h>

#define GEMM_PARAM_SIZE 10

// <<--params-->>
typedef struct {
	uint32_t ninputs;    // Number of inputs
	uint32_t d1;         // Size d1 of the matrix 1
	uint32_t d2;         // Size d2 of the matrix 1
	uint32_t d3;         // Size d2 of the matrix 2
	uint32_t ld_offset1; // Input offset (matrix 1)
	uint32_t ld_offset2; // Input offset (matrix 2)
	uint32_t st_offset;  // Output offset
	uint32_t do_relu; // Do ReLU stage
	uint32_t transpose; // True
	uint32_t padding;
} gemm_params_t;

struct gemm_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	gemm_params_t params;
};

#define GEMM_STRATUS_IOC_ACCESS	_IOWR ('S', 0, struct gemm_stratus_access)

#endif /* _GEMM_STRATUS_H_ */
