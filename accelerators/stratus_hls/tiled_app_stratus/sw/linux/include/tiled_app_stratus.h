// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _TILED_APP_STRATUS_H_
#define _TILED_APP_STRATUS_H_

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


struct tiled_app_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned num_tiles;
	unsigned tile_size;
	unsigned rd_wr_enable;
	unsigned src_offset;
	unsigned dst_offset;	
	unsigned input_spin_sync_offset;
	unsigned output_spin_sync_offset;
	unsigned input_update_sync_offset;
	unsigned output_update_sync_offset;
	unsigned input_tile_start_offset;
	unsigned output_tile_start_offset;
	unsigned spandex_reg;
};

#define TILED_APP_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct tiled_app_stratus_access)

#endif /* _TILED_APP_STRATUS_H_ */
