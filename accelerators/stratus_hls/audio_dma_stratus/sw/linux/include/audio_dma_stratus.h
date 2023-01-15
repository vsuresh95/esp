// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _SM_SENSOR_STRATUS_H_
#define _SM_SENSOR_STRATUS_H_

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

struct audio_dma_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned size;
	unsigned src_offset;
	unsigned dst_offset;
};

#define SM_SENSOR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct audio_dma_stratus_access)

#endif /* _SM_SENSOR_STRATUS_H_ */
