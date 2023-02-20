// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#include "libesp.h"
#include "tiled_app_stratus.h"
#include "coh_func.h"
typedef int64_t token_t;

/* <<--params-def-->> */
#define NUM_TILES 12
// #define TILE_SIZE 1024
#define TILE_SIZE 10
#define RD_WR_ENABLE 0

#define NACC 1

#define SYNC_VAR_SIZE 4

#define NUM_DEVICES 3

/* <<--params-->> */
int32_t num_tiles = NUM_TILES;
int32_t tile_size = TILE_SIZE;
int32_t num_devices = NUM_DEVICES;
const int32_t rd_wr_enable = RD_WR_ENABLE;
// /* User defined registers */
// /* <<--regs-->> */
// #define TILED_APP_OUTPUT_TILE_START_OFFSET_REG 0x60
// #define TILED_APP_INPUT_TILE_START_OFFSET_REG 0x5C
// #define TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG 0x58
// #define TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG 0x54
// #define TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG 0x50
// #define TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG 0x4C
// #define TILED_APP_NUM_TILES_REG 0x48
// #define TILED_APP_TILE_SIZE_REG 0x44
// #define TILED_APP_RD_WR_ENABLE_REG 0x40



token_t *buf;

int regInv;

int32_t accel_read_sync_spin_offset[NUM_DEVICES];
int32_t accel_write_sync_spin_offset[NUM_DEVICES];
int32_t accel_read_sync_write_offset[NUM_DEVICES];
int32_t accel_write_sync_write_offset[NUM_DEVICES];
int32_t input_buffer_offset[NUM_DEVICES];
int32_t output_buffer_offset[NUM_DEVICES];



struct esp_access esp ={
	.coherence = ACC_COH_FULL,
	.p2p_store = 0,
	.p2p_nsrcs = 0,
	.p2p_srcs = {"", "", "", ""},
	.start_stop = 1,
};

struct tiled_app_stratus_access tiled_app_cfg_000[NUM_DEVICES];
// = {
//	{
//		/* <<--descriptor-->> */
//		.num_tiles = NUM_TILES,
//		.tile_size = TILE_SIZE,
//		.rd_wr_enable = RD_WR_ENABLE,
//		// .src_offset = 0,
//		// .dst_offset = 0,
//		.input_spin_sync_offset = 0,
//    	.output_spin_sync_offset = 0,
//    	.input_update_sync_offset = 0,
//    	.output_update_sync_offset = 0,
//    	.input_tile_start_offset = 0, 
//    	.output_tile_start_offset = 0,
//		.esp.coherence = ACC_COH_FULL,
//		.esp.p2p_store = 0,
//		.esp.p2p_nsrcs = 0,
//		.esp.p2p_srcs = {"", "", "", ""},
//	}
//};

esp_thread_info_t cfg_000[NUM_DEVICES];
// // = {
// //	{
// //		.run = true,
// //		.devname = "tiled_app_stratus.0",
// //		.ioctl_req = TILED_APP_STRATUS_IOC_ACCESS,
// //		.esp_desc = &(tiled_app_cfg_000[0].esp),
// //	}
// //};

void update_tiled_app_cfg(int num_devices, int tile_size){
	printf("At file %s func %s line %d\n", __FILE__, __func__, __LINE__);

	int32_t rel_output_buffer_offset = tile_size + 2*SYNC_VAR_SIZE;
	int32_t rel_input_buffer_offset = SYNC_VAR_SIZE;
	int32_t rel_accel_write_sync_write_offset = tile_size + SYNC_VAR_SIZE;
	int32_t rel_accel_read_sync_write_offset = 0;
	int32_t rel_accel_write_sync_spin_offset = rel_accel_write_sync_write_offset;//+2
	int32_t rel_accel_read_sync_spin_offset = rel_accel_read_sync_write_offset;	//+2


	int dev_id;
	for(dev_id = 0; dev_id < num_devices; dev_id++){
		// printf("dev_id=%d\n", dev_id);
		accel_read_sync_spin_offset[dev_id]  = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_read_sync_spin_offset;
		accel_write_sync_spin_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_write_sync_spin_offset;
		accel_read_sync_write_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_read_sync_write_offset;
		accel_write_sync_write_offset[dev_id]= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_write_sync_write_offset;
		input_buffer_offset[dev_id]  		 = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_input_buffer_offset;
		output_buffer_offset[dev_id] 		 = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_output_buffer_offset;

		// printf("At file %s func %s line %d\n", __FILE__, __func__, __LINE__);
		if(regInv)
		tiled_app_cfg_000[dev_id].num_tiles = 1;	
		else
		tiled_app_cfg_000[dev_id].num_tiles = num_tiles;	
		tiled_app_cfg_000[dev_id].tile_size = tile_size;
		tiled_app_cfg_000[dev_id].rd_wr_enable = RD_WR_ENABLE;
		tiled_app_cfg_000[dev_id].input_spin_sync_offset = accel_read_sync_spin_offset[dev_id];
		tiled_app_cfg_000[dev_id].output_spin_sync_offset = accel_write_sync_spin_offset[dev_id];
		tiled_app_cfg_000[dev_id].input_update_sync_offset = accel_read_sync_write_offset[dev_id];
		tiled_app_cfg_000[dev_id].output_update_sync_offset = accel_write_sync_write_offset[dev_id];
		tiled_app_cfg_000[dev_id].input_tile_start_offset = input_buffer_offset[dev_id];
		tiled_app_cfg_000[dev_id].output_tile_start_offset = output_buffer_offset[dev_id];
		tiled_app_cfg_000[dev_id].esp = esp;
		tiled_app_cfg_000[dev_id].esp.coherence = coherence;
		//spandex
		tiled_app_cfg_000[dev_id].spandex_reg = spandex_config.spandex_reg;


		if(regInv)
			tiled_app_cfg_000[dev_id].esp.start_stop = 0;
		// printf("At file %s func %s line %d\n", __FILE__, __func__, __LINE__);

		cfg_000[dev_id].run = true;
		if(dev_id == 0)
			cfg_000[dev_id].devname = "tiled_app_stratus.0";// strcat(DRV_NAME, itostr(dev_id, 10));// 
		if(dev_id == 1)
			cfg_000[dev_id].devname = "tiled_app_stratus.1";// strcat(DRV_NAME, itostr(dev_id, 10));// 
		if(dev_id == 2)
			cfg_000[dev_id].devname = "tiled_app_stratus.2";// strcat(DRV_NAME, itostr(dev_id, 10));// 
		// strcpy(dev_name, cfg_000[dev_id].devname );
		cfg_000[dev_id].ioctl_req = TILED_APP_STRATUS_IOC_ACCESS;
		cfg_000[dev_id].esp_desc = &(tiled_app_cfg_000[dev_id].esp);
		cfg_000[dev_id].hw_buf = buf;
		// printf("At file %s func %s line %d\n", __FILE__, __func__, __LINE__);
	}
}

// esp_thread_info_t cfg_000[] = {
// 	{
// 		.run = true,
// 		.devname = "tiled_app_stratus.0",
// 		.ioctl_req = TILED_APP_STRATUS_IOC_ACCESS,
// 		.esp_desc = &(tiled_app_cfg_000[0].esp),
// 	}
// };

// #if NUM_DEVICES > 0
// esp_thread_info_t cfg_001[] = {
// 	{
// 		.run = true,
// 		.devname = "tiled_app_stratus.1",
// 		.ioctl_req = TILED_APP_STRATUS_IOC_ACCESS,
// 		.esp_desc = &(tiled_app_cfg_000[1].esp),
// 	}
// };

// #if NUM_DEVICES > 1
// esp_thread_info_t cfg_002[] = {
// 	{
// 		.run = true,
// 		.devname = "tiled_app_stratus.2",
// 		.ioctl_req = TILED_APP_STRATUS_IOC_ACCESS,
// 		.esp_desc = &(tiled_app_cfg_000[2].esp),
// 	}
// };
// #endif
// #endif

#endif /* __ESP_CFG_000_H__ */
