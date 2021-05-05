// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _GEMM_BLOCK_STRATUS_H_
#define _GEMM_BLOCK_STRATUS_H_

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

struct gemm_block_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned gemm_m;
	unsigned gemm_n;
	unsigned gemm_k;
	unsigned offset_n;
	unsigned offset_m;
	unsigned gemm_batch;
	unsigned block_size;
	unsigned src_offset;
	unsigned dst_offset;
};

#define GEMM_BLOCK_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct gemm_block_stratus_access)

#endif /* _GEMM_BLOCK_STRATUS_H_ */
