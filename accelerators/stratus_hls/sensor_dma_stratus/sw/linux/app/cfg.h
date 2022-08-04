// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "sensor_dma_stratus.h"

typedef int64_t token_t;

/* <<--params-def-->> */
#define RD_SP_OFFSET 1
#define RD_WR_ENABLE 0
#define WR_SIZE 1
#define WR_SP_OFFSET 1
#define RD_SIZE 1
#define DST_OFFSET 0
#define SRC_OFFSET 0

/* <<--params-->> */
const int32_t rd_sp_offset = RD_SP_OFFSET;
const int32_t rd_wr_enable = RD_WR_ENABLE;
const int32_t wr_size = WR_SIZE;
const int32_t wr_sp_offset = WR_SP_OFFSET;
const int32_t rd_size = RD_SIZE;
const int32_t dst_offset = DST_OFFSET;
const int32_t src_offset = SRC_OFFSET;

typedef union
{
  struct
  {
    unsigned char r_en   : 1;
    unsigned char r_op   : 1;
    unsigned char r_type : 2;
    unsigned char r_cid  : 4;
    unsigned char w_en   : 1;
    unsigned char w_op   : 1;
    unsigned char w_type : 2;
    unsigned char w_cid  : 4;
	uint16_t reserved: 16;
  };
  uint32_t spandex_reg;
} spandex_config_t;

#define NACC 1

#define COH_MODE 0
// #define ESP

#define QUAUX(X) #X
#define QU(X) QUAUX(X)

#ifdef ESP
// ESP COHERENCE PROTOCOLS
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
#else
//SPANDEX COHERENCE PROTOCOLS
#if (COH_MODE == 3)
// Owner Prediction
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2262B82B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_op = 1, .w_type = 1};
#elif (COH_MODE == 2)
// Write-through forwarding
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2062B02B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_type = 1};
#elif (COH_MODE == 1)
// Baseline Spandex
#define READ_CODE 0x2002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 1};
#else
// Fully Coherent MESI
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config;
#endif
#endif

struct sensor_dma_stratus_access sensor_dma_cfg_000[] = {
	{
		/* <<--descriptor-->> */
		.rd_sp_offset = RD_SP_OFFSET,
		.rd_wr_enable = RD_WR_ENABLE,
		.wr_size = WR_SIZE,
		.wr_sp_offset = WR_SP_OFFSET,
		.rd_size = RD_SIZE,
		.sensor_dst_offset = DST_OFFSET,
		.sensor_src_offset = SRC_OFFSET,
		.src_offset = 0,
		.dst_offset = 0,
#ifdef ESP
#if (COH_MODE == 1)
		.esp.coherence = ACC_COH_RECALL,
#else
		.esp.coherence = ACC_COH_FULL,
#endif
#else
		.esp.coherence = ACC_COH_FULL,
#endif
		.esp.p2p_store = 0,
		.esp.p2p_nsrcs = 0,
		.esp.p2p_srcs = {"", "", "", ""},
	}
};

esp_thread_info_t cfg_000[] = {
	{
		.run = true,
		.devname = "sensor_dma_stratus.0",
		.ioctl_req = SENSOR_DMA_STRATUS_IOC_ACCESS,
		.esp_desc = &(sensor_dma_cfg_000[0].esp),
	}
};

#endif /* __ESP_CFG_000_H__ */
