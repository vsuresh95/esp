// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "hu_audiodec_rtl.h"

typedef int32_t token_t;

/* <<--params-def-->> */
#define CFG_REGS_31 1
#define CFG_REGS_30 1
#define CFG_REGS_26 1
#define CFG_REGS_27 1
#define CFG_REGS_24 1
#define CFG_REGS_25 1
#define CFG_REGS_22 1
#define CFG_REGS_23 1
#define CFG_REGS_8 1
#define CFG_REGS_20 1
#define CFG_REGS_9 1
#define CFG_REGS_21 1
#define CFG_REGS_6 1
#define CFG_REGS_7 1
#define CFG_REGS_4 1
#define CFG_REGS_5 1
#define CFG_REGS_2 1
#define CFG_REGS_3 1
#define CFG_REGS_0 1
#define CFG_REGS_28 1
#define CFG_REGS_1 1
#define CFG_REGS_29 1
#define CFG_REGS_19 1
#define CFG_REGS_18 1
#define CFG_REGS_17 1
#define CFG_REGS_16 1
#define CFG_REGS_15 1
#define CFG_REGS_14 1
#define CFG_REGS_13 1
#define CFG_REGS_12 1
#define CFG_REGS_11 1
#define CFG_REGS_10 1

/* <<--params-->> */
const int32_t cfg_regs_31 = CFG_REGS_31;
const int32_t cfg_regs_30 = CFG_REGS_30;
const int32_t cfg_regs_26 = CFG_REGS_26;
const int32_t cfg_regs_27 = CFG_REGS_27;
const int32_t cfg_regs_24 = CFG_REGS_24;
const int32_t cfg_regs_25 = CFG_REGS_25;
const int32_t cfg_regs_22 = CFG_REGS_22;
const int32_t cfg_regs_23 = CFG_REGS_23;
const int32_t cfg_regs_8 = CFG_REGS_8;
const int32_t cfg_regs_20 = CFG_REGS_20;
const int32_t cfg_regs_9 = CFG_REGS_9;
const int32_t cfg_regs_21 = CFG_REGS_21;
const int32_t cfg_regs_6 = CFG_REGS_6;
const int32_t cfg_regs_7 = CFG_REGS_7;
const int32_t cfg_regs_4 = CFG_REGS_4;
const int32_t cfg_regs_5 = CFG_REGS_5;
const int32_t cfg_regs_2 = CFG_REGS_2;
const int32_t cfg_regs_3 = CFG_REGS_3;
const int32_t cfg_regs_0 = CFG_REGS_0;
const int32_t cfg_regs_28 = CFG_REGS_28;
const int32_t cfg_regs_1 = CFG_REGS_1;
const int32_t cfg_regs_29 = CFG_REGS_29;
const int32_t cfg_regs_19 = CFG_REGS_19;
const int32_t cfg_regs_18 = CFG_REGS_18;
const int32_t cfg_regs_17 = CFG_REGS_17;
const int32_t cfg_regs_16 = CFG_REGS_16;
const int32_t cfg_regs_15 = CFG_REGS_15;
const int32_t cfg_regs_14 = CFG_REGS_14;
const int32_t cfg_regs_13 = CFG_REGS_13;
const int32_t cfg_regs_12 = CFG_REGS_12;
const int32_t cfg_regs_11 = CFG_REGS_11;
const int32_t cfg_regs_10 = CFG_REGS_10;

#define NACC 1

struct hu_audiodec_rtl_access hu_audiodec_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.cfg_regs_31 = CFG_REGS_31,
		.cfg_regs_30 = CFG_REGS_30,
		.cfg_regs_26 = CFG_REGS_26,
		.cfg_regs_27 = CFG_REGS_27,
		.cfg_regs_24 = CFG_REGS_24,
		.cfg_regs_25 = CFG_REGS_25,
		.cfg_regs_22 = CFG_REGS_22,
		.cfg_regs_23 = CFG_REGS_23,
		.cfg_regs_8 = CFG_REGS_8,
		.cfg_regs_20 = CFG_REGS_20,
		.cfg_regs_9 = CFG_REGS_9,
		.cfg_regs_21 = CFG_REGS_21,
		.cfg_regs_6 = CFG_REGS_6,
		.cfg_regs_7 = CFG_REGS_7,
		.cfg_regs_4 = CFG_REGS_4,
		.cfg_regs_5 = CFG_REGS_5,
		.cfg_regs_2 = CFG_REGS_2,
		.cfg_regs_3 = CFG_REGS_3,
		.cfg_regs_0 = CFG_REGS_0,
		.cfg_regs_28 = CFG_REGS_28,
		.cfg_regs_1 = CFG_REGS_1,
		.cfg_regs_29 = CFG_REGS_29,
		.cfg_regs_19 = CFG_REGS_19,
		.cfg_regs_18 = CFG_REGS_18,
		.cfg_regs_17 = CFG_REGS_17,
		.cfg_regs_16 = CFG_REGS_16,
		.cfg_regs_15 = CFG_REGS_15,
		.cfg_regs_14 = CFG_REGS_14,
		.cfg_regs_13 = CFG_REGS_13,
		.cfg_regs_12 = CFG_REGS_12,
		.cfg_regs_11 = CFG_REGS_11,
		.cfg_regs_10 = CFG_REGS_10,
		.src_offset = 0,
		.dst_offset = 0,
		.esp.coherence = ACC_COH_NONE,
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "hu_audiodec_rtl.0",
		.ioctl_req = HU_AUDIODEC_RTL_IOC_ACCESS,
		.esp_desc = &(hu_audiodec_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
