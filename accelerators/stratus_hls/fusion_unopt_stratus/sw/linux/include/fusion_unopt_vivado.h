// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _FUSION_UNOPT_VIVADO_H_
#define _FUSION_UNOPT_VIVADO_H_

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

struct fusion_unopt_vivado_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned veno;
	unsigned imgwidth;
	unsigned htdim;
	unsigned imgheight;
	unsigned sdf_block_size;
	unsigned sdf_block_size3;
	unsigned src_offset;
	unsigned dst_offset;
};

#define FUSION_UNOPT_VIVADO_IOC_ACCESS	_IOW ('S', 0, struct fusion_unopt_vivado_access)

#endif /* _FUSION_UNOPT_VIVADO_H_ */
