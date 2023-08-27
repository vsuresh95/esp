// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "hu_audioenc_rtl.h"

typedef int32_t token_t;
typedef float native_t;

/* <<--params-def-->> */
#define NACC 1

#define FX_IL 16

#define BLOCK_SIZE 1024
#define NUM_CHANNELS 16
#define NUM_SRCS 1

enum BFormatChannels3D
{
    kW,
    kY, kZ, kX,
    kV, kT, kR, kS, kU,
    kQ, kO, kM, kK, kL, kN, kP,
    kNumOfBformatChannels3D
};

float cfg_src_coeff[NUM_SRCS];
float cfg_chan_coeff[NUM_CHANNELS];

struct hu_audioenc_rtl_access hu_audioenc_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.cfg_regs_00 = 0,
		.cfg_regs_01 = NUM_SRCS,
		.cfg_regs_02 = BLOCK_SIZE,
		.cfg_regs_03 = 0,
		.cfg_regs_04 = 0,
		.cfg_regs_05 = 0,
		.cfg_regs_06 = 0,
		.cfg_regs_07 = 0,
		.cfg_regs_08 = 0,
		.cfg_regs_09 = 0,
		.cfg_regs_10 = 0,
		.cfg_regs_11 = 0,
		.cfg_regs_12 = 0,
		.cfg_regs_13 = 0,
		.cfg_regs_14 = 0,
		.cfg_regs_15 = 0,
		.cfg_regs_16 = 0,
		.cfg_regs_17 = 0,
		.cfg_regs_18 = 0,
		.cfg_regs_19 = 0,
		.cfg_regs_20 = 0,
		.cfg_regs_21 = 0,
		.cfg_regs_22 = 0,
		.cfg_regs_23 = 0,
		.cfg_regs_24 = 0,
		.cfg_regs_25 = 0,
		.cfg_regs_26 = 0,
		.cfg_regs_27 = 0,
		.cfg_regs_28 = 0,
		.cfg_regs_29 = 0,
		.cfg_regs_30 = 0,
		.cfg_regs_31 = 0,
		.cfg_regs_32 = 0,
		.cfg_regs_33 = 0,
		.cfg_regs_34 = 0,
		.cfg_regs_35 = 0,
		.cfg_regs_36 = 0,
		.cfg_regs_37 = 0,
		.cfg_regs_38 = 0,
		.cfg_regs_39 = 0,
		.cfg_regs_40 = 0,
		.cfg_regs_41 = 0,
		.cfg_regs_42 = 0,
		.cfg_regs_43 = 0,
		.cfg_regs_44 = 0,
		.cfg_regs_45 = 0,
		.cfg_regs_46 = 0,
		.cfg_regs_47 = 0,

		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_RECALL,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "hu_audioenc_rtl.0",
		.ioctl_req = HU_AUDIOENC_RTL_IOC_ACCESS,
		.esp_desc = &(hu_audioenc_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
