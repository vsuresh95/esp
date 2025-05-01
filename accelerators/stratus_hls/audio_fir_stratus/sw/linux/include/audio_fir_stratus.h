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
	unsigned logn_samples;

	// ASI sync flag offsets
    unsigned input_queue_base;
    unsigned output_queue_base;
    unsigned filter_queue_base;

	// Context related registers
	unsigned valid_contexts;
	unsigned context_quota;
};

#define AUDIO_FIR_STRATUS_IOC_ACCESS		_IOW ('S', 0, struct audio_fir_stratus_access)
#define AUDIO_FIR_STRATUS_INIT_IOC_ACCESS	_IOW ('S', 1, struct audio_fir_stratus_access)
#define AUDIO_FIR_STRATUS_ADD_IOC_ACCESS	_IOW ('S', 2, struct audio_fir_stratus_access)
#define AUDIO_FIR_STRATUS_DEL_IOC_ACCESS	_IOW ('S', 3, struct audio_fir_stratus_access)

#endif /* _AUDIO_FIR_STRATUS_H_ */
