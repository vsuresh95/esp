/* Copyright (c) 2011-2021 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

// input /scratch/projects/bmishra3/spandex/esp_tiled_acc/socs/xilinx-vcu118-xcvu9p/restore.tcl.svcf

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#define BAREMETAL_TEST


#include <esp_probe.h>
#include <fixed_point.h>
#include "../linux/app/coh_func.h"
#include "../linux/app/tiled_app.h"
#define QUAUX(X) #X
#define QU(X) QUAUX(X)


// typedef int64_t token_t;

// #define SYNC_VAR_SIZE 4

/* Size of the contiguous chunks for scatter/gather */



static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


uint64_t start_write;
uint64_t stop_write;
uint64_t intvl_write;
uint64_t start_read;
uint64_t stop_read;
uint64_t intvl_read;
uint64_t start_flush;
uint64_t stop_flush;
uint64_t intvl_flush;
uint64_t start_tiling;
uint64_t stop_tiling;
uint64_t intvl_tiling;
uint64_t start_sync;
uint64_t stop_sync;
uint64_t intvl_sync;
uint64_t spin_flush;

uint64_t start_acc_write;
uint64_t stop_acc_write;
uint64_t intvl_acc_write;
uint64_t start_acc_read;
uint64_t stop_acc_read;
uint64_t intvl_acc_read;

uint64_t start_network_cpu[3];
uint64_t stop_network_cpu[3];
uint64_t intvl_network_cpu[3];
uint64_t start_network_acc[3];
uint64_t stop_network_acc[3];
uint64_t intvl_network_acc[3];
uint64_t start_network_llc[3];
uint64_t stop_network_llc[3];
uint64_t intvl_network_llc[3];



/* <<--params-->> */

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;

static unsigned	int read_tile ;
static unsigned	int write_tile;

// static unsigned coherence;

// /* User defined registers */
// /* <<--regs-->> */

static int validate_buf(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;
	int loc = 0;
	for (i = 0; i < num_tiles; i++)
		for (j = 0; j < tile_size; j++){
			loc++;
			if (loc != out[i * out_words_adj + j]){
#ifdef PRINT_DEBUG
				printf("tile: %d loc:%d -- found: %d \n",i, loc, out[i * out_words_adj + j]);
#endif
				errors++;
			}
		}

	return errors;
}


static void init_buf (token_t *in, token_t * gold, token_t* out, int buf_size)
{
	int i;
	int j;
	int offset = 2*tile_size;
	int64_t val_64 = 0;
	void * dst = (void*)(in);
	for (j = 0; j < buf_size; j++){
		// #if (COH_MODE == 0) && !defined(ESP)
		// in[j] = 0;
		// #else
		asm volatile (
			"mv t0, %0;"
			"mv t1, %1;"
			".word " QU(WRITE_CODE)
			: 
			: "r" (dst), "r" (val_64)
			: "t0", "t1", "memory"
		);

		dst += 8;
		// #endif

	}
}


/* Print utilities
*/
inline void console_log_header(){
	int loc_tilesize = tile_size%1024;
	int left = (loc_tilesize%2)?loc_tilesize/2+1 : loc_tilesize/2;
	int right = loc_tilesize/2;

	printf("Tile Size : %d|%d\t||", tile_size, loc_tilesize);
	for (int __i = 0; __i<left; __i++)printf("\t");
	printf("READ TILE");
	for (int __i = 0; __i<right; __i++)printf("\t");
	printf("||");
	for (int __i = 0; __i<left; __i++)printf("\t");
	printf("STORE TILE");
	for (int __i = 0; __i<right; __i++)printf("\t");
	printf("||SYNC VAR\n");
}

inline static void print_mem(int read ){
	int loc_tilesize = tile_size%1024;
	if(read == 1)
	printf("Read  Tile: %d\t||", read_tile);
	else if(read == 0)
	printf("Store Tile: %d\t||", write_tile);
	else
	printf("Polled Tile (%d): %d\t||", ( mem_size/8), write_tile);

	int cntr = 0;
	void* dst = (void*)(mem);
	int row_boundary = accel_cons_valid_offset[0];
	int n = 0;
	for (int j = 0; j < mem_size/8; j++){
		int64_t value_64 = 0;
		asm volatile (
			"mv t0, %1;"
			".word " QU(READ_CODE) ";"
			"mv %0, t1"
			: "=r" (value_64)
			: "r" (dst)
			: "t0", "t1", "memory"
		);
			if(cntr == 0){
				printf("\nInput for Accel %d : \n", n);
			}
			printf("\t%d",value_64);
			if(cntr==SYNC_VAR_SIZE-1){
				printf(" || ");
			}
			// if( j>64 && ((j-64)%tile_size == 0)) printf("\n ==================================\n");
			if( cntr == row_boundary-1){ 
				printf("\n ==================================\n");
				cntr = 0;
				n++;
			}else cntr++;
			dst += 8; 
	// cntr++;
	}
}


int main(int argc, char * argv[])
{
	// printf("Hello World 123\n");
	int32_t num_tiles = NUM_TILES;//12;
	int32_t tile_size = TILE_SIZE;

	// #if 1
	int i;
	int n;
	unsigned done;
	unsigned errors = 0;
	// printf("Checkpoint 1\n");

	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = tile_size;
		out_words_adj = tile_size;
	} else {
		in_words_adj = round_up(tile_size, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(tile_size, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	

	// Search for the device
	//printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_TILED_APP, DEV_NAME);
	if (ndev == 0) {
		printf("tiled_app not found\n");
		return 0;
	}
	ndev = (num_devices<ndev)? num_devices : ndev;
	num_devices = ndev;

	//#ifdef PRINT_DEBUG
	//printf("Devices found:%d\n", ndev);
	//#endif

	in_len = in_words_adj;//+64
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t)*num_devices;
	out_offset  = in_len;
	mem_size = (NUM_DEVICES+1)*(TILE_SIZE + SYNC_VAR_SIZE)* sizeof(token_t);
	dev_mem_size = 2*(TILE_SIZE + SYNC_VAR_SIZE)* sizeof(token_t) ;
	
	//printf("num_tiles      =  %u\n",num_tiles     	); //num_tiles);
	//printf("tile_size      =  %u\n",tile_size	    ); //num_tiles);
	//printf("=== Coherence mode : %s ===\n", print_coh);


	// Allocate memory
	#ifdef VALIDATE
	out = aligned_malloc(out_size*num_tiles+10);
	#endif
	mem = aligned_malloc(mem_size);

	buf = mem;

	set_offsets();
	reset_sync();


	read_tile = 0;
	write_tile = 0;
	// Wait for completion
	done = 0;
	int done_prev = 0;
	int temp_rd_wr_enable = 0;
	int store_done_prev = 0;
	int load_turn = 1;
	
	t_cpu_write=0;
	t_acc=0;
	t_cpu_read=0;
	t_sw=0;
	t_total=0;
	update_tiled_app_cfg(num_devices, tile_size);

	// init_buf(mem, gold, out, mem_size/sizeof(token_t));
	// asm volatile ("fence w, rw");
	
	comp_intensity = COMPUTE_INTENSITY;
	mode = COMP_MODE;
	
//	#ifdef CFA
//	printf("CFA Test\n");
//	#else
//	printf("MA Test\n");
//	#endif

//#ifdef CFA
//	printf("\n Coh_Mode\tnum_dev\tnum_tiles\ttile_size\tcomp_int\tcomp_size\tmode");
//	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, num_devices, num_tiles, tile_size, comp_intensity, comp_size, mode);
//#else
//	printf("\n Coh Mode\tnum_stg\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
//	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, comp_stages, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
//#endif
	
	synth_accel_asi(mode);
	//printf("\n CPURead\tCPU_Write\tAccel_Time\tSync_Time\nTotal_Time");
	//printf("\n %ld\t%ld\t%ld\t%ld\t%ld\n", t_cpu_read/ITERATIONS, t_cpu_write/ITERATIONS, t_acc/ITERATIONS, t_sw/ITERATIONS, t_total/ITERATIONS);
	
	//for(int n_dev = 0; n_dev < num_devices; n_dev++) printf("Last item for %d: %ld(prod) %ld(con)\n",n_dev,buf[accel_prod_valid_offset[n_dev]+1], buf[accel_cons_valid_offset[n_dev]+1]);
	int comparison_val=0;
	#ifdef CFA
	printf("Results: Synthetic ");
	comparison_val = num_devices;
#else
	printf("Results: Mono_Synthetic ");
	comparison_val = comp_stages;
#endif
	if(mode == 0)printf("Linux ");
	else if (mode == 1) printf("Chaining ");
	else if (mode == 2) printf("Pipelining ");
	else if (mode == 5) printf("SW ");
	
	if(comparison_val*comp_intensity>500) printf("Large ");
	else printf("Small ");

	if(mode != 5){	
		printf("%d ", comparison_val);
		if(mode != 0)
		printf("%s ", print_coh);
	}
	printf("=\t%ld\n", t_total/ITERATIONS);

	return 0;
}

