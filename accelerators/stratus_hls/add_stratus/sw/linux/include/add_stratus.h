// Copyright (c) 2011-2022 Columbia University, System Level Design Group
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

struct add_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned do_inverse;
	unsigned logn_samples;
	unsigned do_shift;

	// ASI sync flag offsets
    unsigned input_queue_base;
    unsigned output_queue_base;
};

#define ADD_STRATUS_IOC_ACCESS		_IOW ('S', 0, struct add_stratus_access)
#define ADD_STRATUS_INIT_IOC_ACCESS	_IOW ('S', 1, struct add_stratus_access)
#define ADD_STRATUS_ADD_IOC_ACCESS	_IOW ('S', 2, struct add_stratus_access)
#define ADD_STRATUS_DEL_IOC_ACCESS	_IOW ('S', 3, struct add_stratus_access)
#define ADD_STRATUS_PRIO_IOC_ACCESS	_IOW ('S', 4, struct add_stratus_access)

#endif /* _ADD_STRATUS_H_ */
