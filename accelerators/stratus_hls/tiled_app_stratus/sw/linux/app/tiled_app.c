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

#define ALPHA 1

static unsigned mem_size;
static unsigned dev_mem_size;

static void sw_run(int stride){
	for(int til = 0; til < num_tiles; til++){
	    int64_t temp = get_counter();
	    write_to_cons();
	    int64_t temp2 = get_counter();
	    t_cpu_write += (temp2-temp);

    	    token_t* src = &buf[input_buffer_offset[0]];
    	    token_t* dst = &buf[output_buffer_offset[num_devices-1]];

	    temp = get_counter();
       	    for(int i = 0; i<tile_size; i+=stride){
                int64_t beta = 2;
                int64_t A_I = src[i];
                
                for(int iter = 0; iter<comp_intensity; iter++){
                    beta = beta * A_I + ALPHA;
                }
                
                dst[i] = beta;
            }
	    temp2 = get_counter();
	    t_sw += (temp2-temp);
	    read_from_prod();
	    temp = get_counter();
	    t_cpu_read += (temp-temp2);
	}
}

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
	//reset_sync();
	update_tiled_app_cfg(num_devices, tile_size);
}


int main(int argc, char **argv)
{
	// int errors;
	t_cpu_write = 0;
	t_cpu_read = 0;
	// regInv = 0;
	// 	#ifdef CFA
	// printf("======CFA MODE======\n\n");
	// 	#else
	// printf("======MA MODE======\n\n");
	// 	#endif

	mode = COMP_MODE;
	if(argc>1){
		#ifdef CFA
		num_devices = atoi(argv[1]);
		#else
		comp_stages = atoi(argv[1]);
		#endif
	}
	if(argc>2){
		num_tiles = atoi(argv[2]);
	}
	if(argc>3){
		tile_size = atoi(argv[3]);
		comp_size = tile_size;
	}
	if(argc>4)
		comp_intensity = atoi(argv[4]);
	// if(argc>5)
	// 	stride = atoi(argv[5]);
	if(argc>5)
		comp_size = atoi(argv[5]);
	if(argc>6)
		mode = atoi(argv[6]);
		
	if (num_devices > MAX_DEVICES) num_devices = MAX_DEVICES;
	mem_size = (num_devices+1)*(tile_size + SYNC_VAR_SIZE)* sizeof(token_t);
	dev_mem_size = 2*(tile_size + SYNC_VAR_SIZE)* sizeof(token_t) ;
	buf = (token_t *) esp_alloc(mem_size);

	t_cpu_write=0;
	t_acc=0;
	t_cpu_read=0;
	t_sw=0;
	t_total=0;

	// printf("\n Initializing Parameters ======\n\n");
	init_parameters();
		
	/* <<--print-params-->> */
// #ifdef CFA
// 	printf("\n Coh Mode\tnum_dev\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
// 	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, num_devices, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
// #else
// 	printf("\n Coh Mode\tnum_stg\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
// 	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, comp_stages, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
//
//#endif
	

	int n_dev = 0;
	// int64_t total_time_start = get_counter();
	if (mode == 0)
		synth_accel_asi(mode);
	else if(mode==5){
	 	//printf("\n SW MODE ======\n\n");
		int l_stride = tile_size/comp_size;
		int64_t temp = get_counter();
		for(int iter = 0; iter<ITERATIONS; iter++)
			sw_run(l_stride);
		int64_t temp2 = get_counter();
		t_total = temp2-temp;
	 //	printf("\n SW MODE ======\n\n");
	}
	else
		for(int iter = 0; iter<ITERATIONS; iter++){
			reset_sync();
			synth_accel_asi(mode);
		}
	// int64_t temp2 = get_counter();
	// t_total = temp2 - total_time_start;
#if 0
#ifdef CFA
	printf("\n(CFA)_Coh_Mode\tnum_dev\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
	// printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, num_devices, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
#else
	printf("\n(MA)_Coh_Mode\tnum_stg\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
	// printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", print_coh, comp_stages, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
#endif
	if(mode!=5)
	printf("\tCPURead\tCPU_Write\tAccel_Time\tTotal_Time");
	else
	printf("\tCPURead\tCPU_Write\tSW_Time\tTotal_Time");
#ifdef CFA
	// printf("\n(CFA) Coh Mode\tnum_dev\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d", print_coh, num_devices, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
#else
	// printf("\n(MA) Coh Mode\tnum_stg\tnum_tiles\ttile_size\tcomp_int\tstride\tcomp_size\tmode");
	printf("\n %s\t%d\t%d\t%d\t%d\t%d\t%d\t%d", print_coh, comp_stages, num_tiles, tile_size, comp_intensity, stride, comp_size, mode);
#endif
	if (mode == 0)
		printf("\t%ld\t%ld\t%ld\t%ld\n", t_cpu_read, t_cpu_write, t_acc, t_total);
	else if(mode == 5)
		printf("\t%ld\t%ld\t%ld\t%ld\n", t_cpu_read/ITERATIONS, t_cpu_write/ITERATIONS, t_sw/ITERATIONS, t_total/ITERATIONS);
	else
		printf("\t%ld\t%ld\t%ld\t%ld\n", t_cpu_read/ITERATIONS, t_cpu_write/ITERATIONS, t_acc/ITERATIONS, t_total/ITERATIONS);

#ifdef CFA
	printf("Results: Synthetic ");
#else
	printf("Results: Mono_Synthetic ");
#endif
	if(mode == 5)
		printf("SW ");
	else
		printf("%s ", print_modes[mode]);
	
#ifdef CFA
	if(num_devices*comp_intensity>200)
		printf("Large %d %s =", num_devices, print_coh);
	else
		printf("Small %d %s =", num_devices, print_coh);
#else
	if(comp_stages*comp_intensity>200)
		printf("Large %d %s =", num_devices, print_coh);
	else
		printf("Small %d %s =", num_devices, print_coh);
#endif
	
printf("%ld\n",t_total/ITERATIONS);

#endif
	int comparison_val=0;
	#ifdef CFA
	printf("Result: Synthetic ");
	comparison_val = num_devices;
#else
	printf("Result: Mono_Synthetic ");
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

	
	//for(int n_dev = 0; n_dev < num_devices; n_dev++) printf("Last item for %d: %ld\n",n_dev,buf[accel_cons_valid_offset[n_dev]+1]);
	//// Cleanup
#ifdef BAREMETAL_TEST
	for(n_dev = 0; n_dev < num_devices; n_dev++){
		struct esp_device *dev = &espdevs[n_dev];
		iowrite32(dev, CMD_REG, 0x0);
		aligned_free(ptable[n_dev]);
	}
	// for(n_dev = 0; n_dev < num_devices; n_dev++) aligned_free(ptable[n_dev]);
	aligned_free(ptable);
	aligned_free(mem);
#else
	esp_free(buf);
#endif
	
	return 0; //errors;
}
