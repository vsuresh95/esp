// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _AUDIO_FIR_STRATUS_H_
#define _AUDIO_FIR_STRATUS_H_

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

struct audio_fir_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;

	// ASI sync flag offsets
	unsigned prod_valid_offset;
	unsigned prod_ready_offset;
	unsigned flt_prod_valid_offset;
	unsigned flt_prod_ready_offset;
	unsigned cons_valid_offset;
	unsigned cons_ready_offset;
	unsigned input_offset;
	unsigned flt_input_offset;
	unsigned twd_input_offset;
	unsigned output_offset;

	unsigned src_offset;
	unsigned dst_offset;
    unsigned spandex_conf;
};

#define AUDIO_FIR_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct audio_fir_stratus_access)

#endif /* _AUDIO_FIR_STRATUS_H_ */
