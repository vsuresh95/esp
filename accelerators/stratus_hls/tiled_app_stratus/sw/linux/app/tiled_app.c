// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include "tiled_app.h"
#include "stdlib.h"
// #define ITERATIONS 1


static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;


// uint64_t t_cpu_write;
// uint64_t t_acc;
// uint64_t t_cpu_read;
// uint64_t t_sw;
// uint64_t t_total;


static unsigned mem_size;
static unsigned dev_mem_size;



/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = tile_size;
		out_words_adj = tile_size;
	} else {
		in_words_adj = round_up(tile_size, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(tile_size, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (num_tiles);
	out_len =  out_words_adj * (num_tiles);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
	t_cpu_write = 0;
	t_acc = 0;
	t_cpu_read = 0;
	t_sw = 0;
	t_total =0;

	set_offsets();
	// reset_sync();
	// if(num_devices <=3)
	update_tiled_app_cfg(num_devices, tile_size);
}


int main(int argc, char **argv)
{
	// int errors;
	t_cpu_write = 0;
	t_cpu_read = 0;
	// regInv = 0;
	mode = COMP_MODE;
	if(argc>1)
		num_devices = atoi(argv[1]);

	if(argc>2)
		comp_size = atoi(argv[2]);
	if(argc>3)
		mode = atoi(argv[3]);

	if(argc>4)
		comp_intensity = atoi(argv[4]);

	if(argc>5){
		num_tiles = atoi(argv[5]);
	}
	if(argc>6){
		tile_size = atoi(argv[6]);
	}
	// if(argc>5)
	// 	stride = atoi(argv[5]);
		
	if (num_devices > MAX_DEVICES) num_devices = MAX_DEVICES;
	mem_size = (num_devices+1)*(tile_size + SYNC_VAR_SIZE)* sizeof(token_t);
	dev_mem_size = 2*(tile_size + SYNC_VAR_SIZE)* sizeof(token_t) ;
	buf = (token_t *) esp_alloc(mem_size);

	printf("\nAllocated %d memory for %d/%d\n", mem_size, num_devices, (int)(MAX_DEVICES));

	t_cpu_write=0;
	t_acc=0;
	t_cpu_read=0;
	t_sw=0;
	t_total=0;

	// printf("\n Initializing Parameters ======\n\n");
	init_parameters();
		
	/* <<--print-params-->> */
	printf("\n Coh Mode\tnum_dev\tnum_tiles\ttile_size\tcomp_int\tcomp_size\tmode");
	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, num_devices, num_tiles, tile_size, comp_intensity, comp_size, mode);
	

	// int n_dev = 0;
	// int64_t total_time_start = get_counter();
	for(int iter = 0; iter<ITERATIONS; iter++){
		// if(num_devices <=3)
		synth_accel_asi(mode);
	}
	// int64_t temp2 = get_counter();
	// t_total = temp2 - total_time_start;

	printf("\n CPURead\tCPU_Write\tAccel_Time\tTotal_Time");
	printf("\n %ld\t%ld\t%ld\t%ld\n", t_cpu_read/ITERATIONS, t_cpu_write/ITERATIONS, t_acc/ITERATIONS, t_total/ITERATIONS);
	
	// for(int n_dev = 0; n_dev < num_devices; n_dev++) printf("Last item for %d: %ld\n",n_dev,buf[accel_cons_valid_offset[n_dev]+1]);
	
	return 0; //errors;
}
