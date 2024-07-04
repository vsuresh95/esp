// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef _ASI_VITDODEC_STRATUS_H_
#define _ASI_VITDODEC_STRATUS_H_

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

struct asi_vitdodec_stratus_access {
	struct esp_access esp;
	/* <<--regs-->> */
	unsigned cbps;
	unsigned ntraceback;
	unsigned data_bits;
	// unsigned src_offset;
	// unsigned dst_offset;
    unsigned in_length 			;
    unsigned out_length 			;
    unsigned input_start_offset 	;
    unsigned output_start_offset 	;
    unsigned accel_cons_vld_offset ;
    unsigned accel_prod_rdy_offset ;
    unsigned accel_cons_rdy_offset ;
    unsigned accel_prod_vld_offset ;
	unsigned spandex_reg;
};

#define ASI_VITDODEC_STRATUS_IOC_ACCESS	_IOW ('S', 0, struct asi_vitdodec_stratus_access)

#endif /* _ASI_VITDODEC_STRATUS_H_ */
