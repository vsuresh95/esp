// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _BASIC_256_GEMM_STRATUS_H_
#define _BASIC_256_GEMM_STRATUS_H_

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

struct basic_256_gemm_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned C_mat_offset;
	unsigned B_mat_offset;
	unsigned A_mat_offset;
	unsigned K;
	unsigned M;
	unsigned N;
	unsigned src_offset;
	unsigned dst_offset;
};

#define BASIC_256_GEMM_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct basic_256_gemm_stratus_access)

#endif /* _BASIC_256_GEMM_STRATUS_H_ */
