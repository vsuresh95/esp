// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _GEMM_STRATUS_H_
#define _GEMM_STRATUS_H_

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

struct gemm_stratus_access {
	struct esp_access esp;
};

#define GEMM_STRATUS_IOC_ACCESS			_IOW ('S', 0, struct gemm_stratus_access)
#define GEMM_STRATUS_RESET_IOC_ACCESS	_IOW ('S', 1, struct gemm_stratus_access)
#define GEMM_STRATUS_INIT_IOC_ACCESS	_IOW ('S', 2, struct gemm_stratus_access)
#define GEMM_STRATUS_ADD_IOC_ACCESS		_IOW ('S', 3, struct gemm_stratus_access)
#define GEMM_STRATUS_DEL_IOC_ACCESS		_IOW ('S', 4, struct gemm_stratus_access)
#define GEMM_STRATUS_PRIO_IOC_ACCESS	_IOW ('S', 5, struct gemm_stratus_access)

#endif /* _GEMM_STRATUS_H_ */
