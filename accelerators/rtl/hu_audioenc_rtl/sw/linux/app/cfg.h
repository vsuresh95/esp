// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "hu_audiodec_rtl.h"

typedef int32_t token_t;
typedef float native_t;

/* <<--params-def-->> */
#define CFG_REGS_24 1
#define CFG_REGS_25 1
#define CFG_REGS_22 1
#define CFG_REGS_23 1
#define CFG_REGS_8 1
#define CFG_REGS_20 1
#define CFG_REGS_9 1
#define CFG_REGS_21 1
#define CFG_REGS_4 1
#define CFG_REGS_2 1
#define CFG_REGS_3 1
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

#define NACC 1

#define FX_IL 16

#define BLOCK_SIZE 1024
#define NUM_CHANNELS 16

enum BFormatChannels3D
{
    kW,
    kY, kZ, kX,
    kV, kT, kR, kS, kU,
    kQ, kO, kM, kK, kL, kN, kP,
    kNumOfBformatChannels3D
};

float m_fCosAlpha;
float m_fSinAlpha;
float m_fCosBeta;
float m_fSinBeta;
float m_fCosGamma;
float m_fSinGamma;
float m_fCos2Alpha;
float m_fSin2Alpha;
float m_fCos2Beta;
float m_fSin2Beta;
float m_fCos2Gamma;
float m_fSin2Gamma;
float m_fCos3Alpha;
float m_fSin3Alpha;
float m_fCos3Beta;
float m_fSin3Beta;
float m_fCos3Gamma;
float m_fSin3Gamma;

struct hu_audiodec_rtl_access hu_audiodec_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.cfg_regs_24 = CFG_REGS_24,
		.cfg_regs_25 = CFG_REGS_25,
		.cfg_regs_22 = CFG_REGS_22,
		.cfg_regs_23 = CFG_REGS_23,
		.cfg_regs_8 = CFG_REGS_8,
		.cfg_regs_20 = CFG_REGS_20,
		.cfg_regs_9 = CFG_REGS_9,
		.cfg_regs_21 = CFG_REGS_21,
		.cfg_regs_4 = 0,
		.cfg_regs_2 = 0,
		.cfg_regs_3 = 0,
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
		.esp.coherence = ACC_COH_RECALL,
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
