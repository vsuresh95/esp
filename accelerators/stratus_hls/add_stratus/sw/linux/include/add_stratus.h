// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _ADD_STRATUS_H_
#define _ADD_STRATUS_H_

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

#define ADD_PARAM_SIZE 6

// <<--params-->>
typedef struct {
	unsigned total_len;
	unsigned input1_offset;
	unsigned input2_offset;
	unsigned output_offset;
	unsigned do_relu;
} add_params_t;

struct add_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	add_params_t params;
};

#define ADD_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct add_stratus_access)

#endif /* _ADD_STRATUS_H_ */
