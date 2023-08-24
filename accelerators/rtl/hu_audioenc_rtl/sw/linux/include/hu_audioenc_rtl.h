// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _HU_AUDIOENC_RTL_H_
#define _HU_AUDIOENC_RTL_H_

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

struct hu_audioenc_rtl_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned cfg_regs_00;
	unsigned cfg_regs_01;
	unsigned cfg_regs_02;
	unsigned cfg_regs_03;
	unsigned cfg_regs_04;
	unsigned cfg_regs_05;
	unsigned cfg_regs_06;
	unsigned cfg_regs_07;
	unsigned cfg_regs_08;
	unsigned cfg_regs_09;
	unsigned cfg_regs_10;
	unsigned cfg_regs_11;
	unsigned cfg_regs_12;
	unsigned cfg_regs_13;
	unsigned cfg_regs_14;
	unsigned cfg_regs_15;
	unsigned cfg_regs_16;
	unsigned cfg_regs_17;
	unsigned cfg_regs_18;
	unsigned cfg_regs_19;
	unsigned cfg_regs_20;
	unsigned cfg_regs_21;
	unsigned cfg_regs_22;
	unsigned cfg_regs_23;
	unsigned cfg_regs_24;
	unsigned cfg_regs_25;
	unsigned cfg_regs_26;
	unsigned cfg_regs_27;
	unsigned cfg_regs_28;
	unsigned cfg_regs_29;
	unsigned cfg_regs_30;
	unsigned cfg_regs_31;
	unsigned cfg_regs_32;
	unsigned cfg_regs_33;
	unsigned cfg_regs_34;
	unsigned cfg_regs_35;
	unsigned cfg_regs_36;
	unsigned cfg_regs_37;
	unsigned cfg_regs_38;
	unsigned cfg_regs_39;
	unsigned cfg_regs_40;
	unsigned cfg_regs_41;
	unsigned cfg_regs_42;
	unsigned cfg_regs_43;
	unsigned cfg_regs_44;
	unsigned cfg_regs_45;
	unsigned cfg_regs_46;
	unsigned cfg_regs_47;
	unsigned src_offset;
	unsigned dst_offset;
};

#define HU_AUDIOENC_RTL_IOC_ACCESS	_IOW ('S', 0, struct hu_audioenc_rtl_access)

#endif /* _HU_AUDIOENC_RTL_H_ */
