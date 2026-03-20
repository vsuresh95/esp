// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _CONV2D_STRATUS_H_
#define _CONV2D_STRATUS_H_

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

#define CONV_PARAM_SIZE 16

// <<--params-->>
typedef struct {
	unsigned n_channels;
	unsigned feature_map_height;
	unsigned feature_map_width;
	unsigned n_filters;
	unsigned filter_height;
	unsigned filter_width;
	unsigned is_padded;
	unsigned stride;
	unsigned do_relu;
	unsigned pool_type;
	unsigned batch_size;
	unsigned input_offset;
	unsigned filters_offset;
	unsigned bias_offset;
	unsigned output_offset;
	unsigned padding;
} conv2d_params_t;

struct conv2d_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	conv2d_params_t params;
};

#define CONV2D_STRATUS_IOC_ACCESS _IOW ('S', 0, struct conv2d_stratus_access)

#endif /* _CONV2D_STRATUS_H_ */
