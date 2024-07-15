// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#ifndef __ESP_CFG_000_H__
#define __ESP_CFG_000_H__

#ifndef BAREMETAL_TEST
#include "libesp.h"
#include "tiled_app_stratus.h"

// else


#endif

#include "stdlib.h"
#include "coh_func.h"
typedef int64_t token_t;

/* <<--params-def-->> */
// Comp Mode 0: Reg Inv
// Comp Mode 1: Chaining
// Comp Mode 2: Pipelining
#define MODE_REG_INV 0
#define MODE_CHAIN 1
#define MODE_PIPE 2

const char* print_modes[]= {"Linux", "Chaining", "Pipelining"};

//#define COMP_MODE MODE_PIPE
#define NUM_TILES 1024
#define TILE_SIZE 1024
//#define TILE_SIZE 1024
#define RD_WR_ENABLE 0
//#define COMPUTE_INTENSITY (150)
//#define COMPUTE_INTENSITY (100)
//75 50 30
#define COMPUTE_SIZE (TILE_SIZE)
// #define TIMERS 1
#define ITERATIONS 1
#define MAX_DEVICES 15

//#define CFA

#define NACC 1

#define SYNC_VAR_SIZE 6


#ifdef CFA
#define COMPUTE_STAGES 1
//#define NUM_DEVICES 3
//#define NUM_DEVICES 1
#else
//#define COMPUTE_STAGES 15
#define NUM_DEVICES 1
//#define MAX_DEVICES 3
#endif


#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* <<--params-->> */
int32_t num_tiles = NUM_TILES;
int32_t tile_size = TILE_SIZE;
int32_t num_devices = NUM_DEVICES;
int32_t comp_intensity = COMPUTE_INTENSITY;
int32_t comp_size = COMPUTE_SIZE;
int32_t comp_stages = COMPUTE_STAGES;
int32_t stride = 1;
int32_t mode = COMP_MODE;
const int32_t rd_wr_enable = RD_WR_ENABLE;

token_t *buf;

// int regInv;

int32_t input_buffer_offset[MAX_DEVICES];
char accel_names[MAX_DEVICES][25];
int32_t output_buffer_offset[MAX_DEVICES];

int32_t accel_prod_last_element[MAX_DEVICES];
int32_t accel_cons_last_element[MAX_DEVICES];

int32_t accel_prod_valid_offset[MAX_DEVICES];
int32_t accel_cons_ready_offset[MAX_DEVICES];
int32_t accel_prod_ready_offset[MAX_DEVICES];
int32_t accel_cons_valid_offset[MAX_DEVICES];

int32_t * cpu_cons_ready_offset = &accel_prod_ready_offset[0];
int32_t * cpu_cons_valid_offset = &accel_prod_valid_offset[0];
int32_t * cpu_prod_ready_offset;// = &accel_cons_ready_offset[NUM_DEVICES-1];
int32_t * cpu_prod_valid_offset;// = &accel_cons_valid_offset[NUM_DEVICES-1];


#ifndef BAREMETAL_TEST
#define NACC 1

struct esp_access esp ={
	.coherence = ACC_COH_FULL,
	.p2p_store = 0,
	.p2p_nsrcs = 0,
	.p2p_srcs = {"", "", "", ""},
	.start_stop = 1,
};

struct tiled_app_stratus_access tiled_app_cfg_000[MAX_DEVICES];

esp_thread_info_t cfg_000[MAX_DEVICES];

void update_tiled_app_cfg(int num_devices, int tile_size){
	int dev_id;
	for(dev_id = 0; dev_id < num_devices; dev_id++){

		if(mode==MODE_REG_INV)
		tiled_app_cfg_000[dev_id].num_tiles = 1;	
		else
		tiled_app_cfg_000[dev_id].num_tiles = num_tiles;	
		tiled_app_cfg_000[dev_id].tile_size = tile_size;
		tiled_app_cfg_000[dev_id].rd_wr_enable = RD_WR_ENABLE;
		tiled_app_cfg_000[dev_id].input_spin_sync_offset = accel_prod_valid_offset[dev_id];
		tiled_app_cfg_000[dev_id].output_spin_sync_offset = accel_cons_ready_offset[dev_id];
		tiled_app_cfg_000[dev_id].input_update_sync_offset = accel_prod_ready_offset[dev_id];
		tiled_app_cfg_000[dev_id].output_update_sync_offset = accel_cons_valid_offset[dev_id];
		tiled_app_cfg_000[dev_id].input_tile_start_offset = input_buffer_offset[dev_id];
		tiled_app_cfg_000[dev_id].output_tile_start_offset = output_buffer_offset[dev_id];
		tiled_app_cfg_000[dev_id].num_comp_units_reg = comp_stages;
		tiled_app_cfg_000[dev_id].esp = esp;
		tiled_app_cfg_000[dev_id].esp.coherence = coherence;
		//spandex
		tiled_app_cfg_000[dev_id].spandex_reg = spandex_config.spandex_reg;

		tiled_app_cfg_000[dev_id].compute_iters = comp_intensity;
		tiled_app_cfg_000[dev_id].compute_over_data = comp_size;
		tiled_app_cfg_000[dev_id].ping_pong_en = 0;

		if(mode==MODE_REG_INV)
			tiled_app_cfg_000[dev_id].esp.start_stop = 0;

		cfg_000[dev_id].run = true;
	//	char namebuf[21];//="tiled_app_stratus.";
		snprintf ( accel_names[dev_id], 25, "tiled_app_stratus.%d",dev_id );
	//	itoa(dev_id, namebuf+18, 10);
	//	strcpy( cfg_000[dev_id].devname,namebuf);// = namebuf;
		cfg_000[dev_id].devname = accel_names[dev_id];
		//printf("%s\n",cfg_000[dev_id].devname);
		//if(dev_id == 0)
		//	cfg_000[dev_id].devname = "tiled_app_stratus.0";
		//if(dev_id == 1)
		//	cfg_000[dev_id].devname = "tiled_app_stratus.1";
		//if(dev_id == 2)
		//	cfg_000[dev_id].devname = "tiled_app_stratus.2";
		cfg_000[dev_id].ioctl_req = TILED_APP_STRATUS_IOC_ACCESS;
		cfg_000[dev_id].esp_desc = &(tiled_app_cfg_000[dev_id].esp);
		cfg_000[dev_id].hw_buf = buf;
	}
}
#else

#include <esp_accelerator.h>


#define SYNC_BITS 1
#define SLD_TILED_APP 0x033
#define DEV_NAME "sld,tiled_app_stratus"


/* User defined registers */
/* <<--regs-->> */
//#define TILED_APP_OUTPUT_TILE_START_OFFSET_REG 0x60
//#define TILED_APP_INPUT_TILE_START_OFFSET_REG 0x5C
//#define TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG 0x58
//#define TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG 0x54
//#define TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG 0x50
//#define TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG 0x4C
//#define TILED_APP_NUM_TILES_REG 0x48
//#define TILED_APP_TILE_SIZE_REG 0x44
//#define TILED_APP_RD_WR_ENABLE_REG 0x40
//// New registers
//#define TILED_APP_PING_PONG_EN_REG 0x6C
//#define TILED_APP_COMPUTE_OVER_DATA_REG 0x68
//#define TILED_APP_COMPUTE_ITERS_REG 0x64
//<param name="num_comp_units" desc="num_comp_units" />
//   <param name="rd_wr_enable" desc="rd_wr_enable" />
//   <param name="tile_size" desc="tile_size" />
//   <param name="num_tiles" desc="num_tiles" />
//   <param name="prod_valid_offset" desc="prod_valid_offset" />
//   <param name="cons_ready_offset" desc="cons_ready_offset" />
//   <param name="prod_ready_offset" desc="prod_ready_offset" />
//   <param name="cons_valid_offset" desc="cons_valid_offset" />
//   <param name="input_offset" desc="input_offset" />
//   <param name="output_offset" desc="output_offset" />
//   <param name="compute_over_data" desc="compute_over_data" />
//   <param name="compute_iters" desc="compute_iters" />
//   <param name="ping_pong_en" desc="ping_pong_en" />

/* <<--regs-->> */
#define TILED_APP_NUM_COMP_UNITS_REG 0x40
#define TILED_APP_RD_WR_ENABLE_REG 0x44
#define TILED_APP_TILE_SIZE_REG 0x48
#define TILED_APP_NUM_TILES_REG 0x4C
#define TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG 0x50
#define TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG 0x54
#define TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG 0x58
#define TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG 0x5C
#define TILED_APP_INPUT_TILE_START_OFFSET_REG 0x60
#define TILED_APP_OUTPUT_TILE_START_OFFSET_REG 0x64
#define TILED_APP_COMPUTE_OVER_DATA_REG 0x68
#define TILED_APP_COMPUTE_ITERS_REG 0x6C
#define TILED_APP_PING_PONG_EN_REG 0x70

int ndev;
struct esp_device *espdevs;
unsigned **ptable;
unsigned **ptable_list[NUM_DEVICES];
unsigned mem_size;
unsigned dev_mem_size;

typedef int64_t token_t;

token_t *mem;
token_t **mem_list;
token_t *gold;
token_t *out;

int update_tiled_app_cfg(int num_devices, int tile_size){
	struct esp_device *dev;
	cpu_prod_ready_offset = &accel_cons_ready_offset[num_devices-1];
	cpu_prod_valid_offset = &accel_cons_valid_offset[num_devices-1];
	for (int n = 0; n < num_devices; n++) {

	//	printf("**************** %s.%d ****************\n", DEV_NAME, n);

		dev = &espdevs[n];

		// Check DMA capabilities
		if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
			printf("  -> scatter-gather DMA is disabled. Abort.\n");
			return 0;
		}

		if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
			printf("  -> Not enough TLB entries available. Abort.\n");
			return 0;
		}

		#ifdef VALIDATE
		printf("  out buffer base-address = %p\n", out);
		#endif
	//	printf("  memory buffer base-address = %p\n", mem);

		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		unsigned int dev_mem_layout_offset = (n*(tile_size + SYNC_VAR_SIZE)); //basically don't include input tile and input sync for previous accelerator
		
		for (int i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];
		ptable_list[n] = ptable;

	//	printf("  ptable = %p\n", ptable);
	//	printf("  nchunk = %lu\n", NCHUNK(mem_size));

        	asm volatile ("fence w, rw");

		// If Spandex Caches
		#ifndef ESP
		// set_spandex_config_reg();
		#if(COH_MODE==3)
			spandex_config.w_cid = (n+2)%(NUM_DEVICES+1);
		#endif
	//	printf("Writing spandex :%d\n", spandex_config.spandex_reg);
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
		#endif

		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);

#ifndef __sparc
		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
#else
		iowrite32(dev, PT_ADDRESS_REG, (unsigned) ptable);
#endif
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 0x0);
		iowrite32(dev, DST_OFFSET_REG, 0x0);

		iowrite32(dev, TILED_APP_OUTPUT_TILE_START_OFFSET_REG ,  output_buffer_offset[n]);
		iowrite32(dev, TILED_APP_INPUT_TILE_START_OFFSET_REG  ,  input_buffer_offset[n]);
		iowrite32(dev, TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG,  accel_cons_valid_offset[n]);
		iowrite32(dev, TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG ,  accel_prod_ready_offset[n]);
		iowrite32(dev, TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG  ,  accel_cons_ready_offset[n]); //
		iowrite32(dev, TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG   ,  accel_prod_valid_offset[n]);

		#ifndef REG_INV
		iowrite32(dev, TILED_APP_NUM_TILES_REG, num_tiles);//ndev-n- +ndev-n-1
		#else
		iowrite32(dev, TILED_APP_NUM_TILES_REG, 1);//ndev-n- +ndev-n-1
		#endif
		iowrite32(dev, TILED_APP_TILE_SIZE_REG, tile_size);
		iowrite32(dev, TILED_APP_RD_WR_ENABLE_REG, n);//device number
		iowrite32(dev, TILED_APP_PING_PONG_EN_REG, 0);
		iowrite32(dev, TILED_APP_COMPUTE_OVER_DATA_REG, comp_size);
		iowrite32(dev, TILED_APP_COMPUTE_ITERS_REG, comp_intensity);
		iowrite32(dev, TILED_APP_NUM_COMP_UNITS_REG, comp_stages);

		// Flush (customize coherence model here)
		esp_flush(coherence);


	}
}

#endif

#endif /* __ESP_CFG_000_H__ */
