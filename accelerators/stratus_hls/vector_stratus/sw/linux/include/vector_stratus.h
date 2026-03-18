// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _VECTOR_STRATUS_H_
#define _VECTOR_STRATUS_H_

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

#define VECTOR_PARAM_SIZE 8

// <<--params-->>
typedef struct {
	unsigned vector_op;
	unsigned n_channel;
	unsigned input_len;
	unsigned stride;
	unsigned input1_offset;
	unsigned input2_offset;
	unsigned output_offset;
	unsigned do_relu;
} vector_params_t;

struct vector_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	vector_params_t params;
};

#define VECTOR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct vector_stratus_access)

#endif /* _VECTOR_STRATUS_H_ */
