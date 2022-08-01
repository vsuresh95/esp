// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _HU_AUDIODEC_48_64_RTL_H_
#define _HU_AUDIODEC_48_64_RTL_H_

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

struct hu_audiodec_48_64_rtl_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned cfg_regs_31;
	unsigned cfg_regs_30;
	unsigned cfg_regs_26;
	unsigned cfg_regs_27;
	unsigned cfg_regs_24;
	unsigned cfg_regs_25;
	unsigned cfg_regs_22;
	unsigned cfg_regs_23;
	unsigned cfg_regs_8;
	unsigned cfg_regs_20;
	unsigned cfg_regs_9;
	unsigned cfg_regs_21;
	unsigned cfg_regs_6;
	unsigned cfg_regs_7;
	unsigned cfg_regs_4;
	unsigned cfg_regs_5;
	unsigned cfg_regs_2;
	unsigned cfg_regs_3;
	unsigned cfg_regs_0;
	unsigned cfg_regs_28;
	unsigned cfg_regs_1;
	unsigned cfg_regs_29;
	unsigned cfg_regs_19;
	unsigned cfg_regs_18;
	unsigned cfg_regs_17;
	unsigned cfg_regs_16;
	unsigned cfg_regs_15;
	unsigned cfg_regs_14;
	unsigned cfg_regs_13;
	unsigned cfg_regs_12;
	unsigned cfg_regs_11;
	unsigned cfg_regs_10;
	unsigned src_offset;
	unsigned dst_offset;
};

#define HU_AUDIODEC_48_64_RTL_IOC_ACCESS	_IOW ('S', 0, struct hu_audiodec_48_64_rtl_access)

#endif /* _HU_AUDIODEC_48_64_RTL_H_ */
