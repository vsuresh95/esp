// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include "stdlib.h"
#define ITERATIONS 100

// #include "sw_func.h"

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;


uint64_t t_cpu_write;
uint64_t t_acc;
uint64_t t_cpu_read;
uint64_t t_sw;
uint64_t t_total;
// 1


void set_spandex_config_reg(){
	#if (COH_MODE == 3)
	// Owner Prediction
	spandex_config.spandex_reg = 0;
	spandex_config.r_en = 1;
	spandex_config.r_type = 2;
	spandex_config.w_en = 1;
	spandex_config.w_op = 1;
	spandex_config.w_type = 1;
	#elif (COH_MODE == 2)
	// Write-through forwarding
	spandex_config.spandex_reg = 0;
	spandex_config.r_en = 1;
	spandex_config.r_type = 2;
	spandex_config.w_en = 1;
	spandex_config.w_type = 1;
	#elif (COH_MODE == 1)
	// Baseline Spandex
	spandex_config.spandex_reg = 0;
	spandex_config.r_en = 1;
	spandex_config.r_type = 1;
	#else
	// Fully Coherent MESI
	spandex_config.spandex_reg = 0;
	#endif
}

static unsigned mem_size;
static unsigned dev_mem_size;

// static unsigned int read_tile;
// static unsigned int write_tile;

/* For performance profiling */
// static inline
static inline uint64_t get_counter() {
    uint64_t t_end = 0;
#ifndef __linux__
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#else
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#endif
	return t_end;
}


/* Write to the memory*/
static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

/* Read from the memory*/
static inline int64_t read_mem (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

static inline void reset_sync(){
	int n;
	for(n = 0; n< num_devices; n++){
		#if !defined(ESP) && COH_MODE == 0
		buf[accel_read_sync_write_offset[n]] = 0;
		buf[accel_write_sync_write_offset[n]] = 0;
		buf[accel_read_sync_spin_offset[n]] = 0;
		buf[accel_write_sync_spin_offset[n]] = 0;
		#else
		// void* dst = ((void*)(buf + accel_read_sync_write_offset[n]));
		write_mem(((void*)(buf + accel_read_sync_write_offset[n])), 0);
		write_mem(((void*)(buf + accel_write_sync_write_offset[n])), 0);
		write_mem(((void*)(buf + accel_read_sync_spin_offset[n])), 0);
		write_mem(((void*)(buf + accel_write_sync_spin_offset[n])), 0);
		#endif
	}
	asm volatile ("fence w, w");	
}

static inline uint32_t read_sync(){

	// void* dst = (void*)(buf);
	// void* dst = (void*)(buf + accel_read_sync_write_offset[0]);
	int64_t value_64 = 0;
	#if !defined(ESP) && COH_MODE == 0
	// #if 1
	//!defined(ESP) && COH_MODE == 0
	// value_64 = buf[num_dev];
	value_64 = buf[accel_read_sync_write_offset[0]];
	#else
	void* dst = (void*)(buf + accel_read_sync_write_offset[0]);
	value_64 = read_mem(dst);
	#endif
	return (value_64 == 0);
}

static inline uint32_t write_sync(){
	// void* dst = (void*)(buf+num_dev);
	void* dst = (void*)(buf+accel_write_sync_write_offset[num_devices-1]);
	int64_t value_64 = 0;
	#if !defined(ESP) && COH_MODE == 0
	// value_64 = buf[num_dev];
	value_64 = buf[accel_write_sync_write_offset[num_devices-1]]; 
	#else
	value_64 = read_mem(dst);
	#endif
	return (value_64 == 1);
}

static inline void update_load_sync(){
	#ifdef PRINT_DEBUG
	printf("Inside update load 1\n");
	#endif
	// void* dst = (void*)(buf);
	void* dst = (void*)(buf+accel_read_sync_spin_offset[0]);
	int64_t value_64 = 1;
	#if !defined(ESP) && COH_MODE == 0
//(COH_MODE == 0) && 
	// buf[0] = value_64;
	buf[accel_read_sync_spin_offset[0]] = value_64;
	#else
	write_mem(dst, value_64);
	#endif

	#ifdef PRINT_DEBUG
	printf("Inside update load 2\n");
	#endif
	//Fence to push the write out from the write buffer
	asm volatile ("fence w, w");	
	// stop_write = get_counter();
	// intvl_write += stop_write - start_write;
	// start_sync = stop_write ; //get_counter();

	#ifdef PRINT_DEBUG
	printf("Inside update load 3\n");
	#endif
}

static inline void update_store_sync(){
	// void* dst = (void*)(buf+num_dev);
	void* dst = (void*)(buf+accel_write_sync_spin_offset[num_devices-1]);
	int64_t value_64 = 0; //Finished reading store_tile
	#if !defined(ESP) && COH_MODE == 0
//(COH_MODE == 0) && 
	// buf[0+num_dev] = value_64
	buf[accel_write_sync_spin_offset[num_devices-1]] = value_64;;
	#else
	write_mem(dst, value_64);
	#endif

	//Fence to push the write out from the write buffer
	asm volatile ("fence w, w");	
	// stop_read = get_counter();
	// intvl_read += stop_read - start_read;
	// start_sync = stop_read ; //get_counter();
}

static inline void load_mem(){
	// void *dst = (void*)(&buf[64]);
	void *dst = (void*)(&buf[input_buffer_offset[0]]);
	// static int64_t temp = 1;
	int64_t val_64 = 123; //(temp++);
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
		#if (COH_MODE == 0) && !defined(ESP)
			buf[input_buffer_offset[0] + j] =val_64; 
		#else
		write_mem(dst, val_64);
		dst += 8;
		#endif
#ifdef VALIDATE
		// val_64++;
#endif
	}
	asm volatile ("fence w, w");
	// read_tile += ÷/1;
}


static inline void store_mem_hit(){
	void *src = (void*)(buf+input_buffer_offset[0]);
	int64_t out_val;
#if defined(VALIDATE) || defined(MEM_DUMP)
	static int64_t curTile = 0; //write_tile*tile_size;
#endif
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
		#if ((COH_MODE == 0) && !defined(ESP))
			// buf[64+j] = val_64;i
			out_val = buf[input_buffer_offset[0] + j]; 
		#else
			out_val = read_mem(src);
			src += 8;
		#endif
	}
	
	asm volatile ("fence w, w");
	// write_tile++;
}


static inline void store_mem(){
	void *src = (void*)(buf+output_buffer_offset[num_devices-1]);
	int64_t out_val;
#if defined(VALIDATE) || defined(MEM_DUMP)
	static int64_t curTile = 0; //write_tile*tile_size;
#endif
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
		#if ((COH_MODE == 0) && !defined(ESP))
			// buf[64+j] = val_64;i
			out_val = buf[output_buffer_offset[num_devices-1] + j]; 
		#else
			out_val = read_mem(src);
			src += 8;
		#endif
	}
	
	asm volatile ("fence w, w");
	// write_tile++;
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

	// #ifndef ESP
	// set_spandex_config_reg();
	// #endif
	update_tiled_app_cfg(num_devices, tile_size);
}


int main(int argc, char **argv)
{
	// int errors;
	t_cpu_write = 0;
	t_cpu_read = 0;
	regInv = 0;
	int pipeline = 1;
	if(argc>1)
		num_devices = atoi(argv[1]);
	if(argc>2)
		num_tiles = atoi(argv[2]);
	if(argc>3)
		tile_size = atoi(argv[3]);
	if(argc>4)
		pipeline = atoi(argv[4]);
	if(argc>5)
		regInv = atoi(argv[5]);
		
	mem_size = (num_devices+1)*(tile_size + SYNC_VAR_SIZE)* sizeof(token_t);
	dev_mem_size = 2*(tile_size + SYNC_VAR_SIZE)* sizeof(token_t) ;
	buf = (token_t *) esp_alloc(mem_size);


	printf("\n Initializing Parameters ======\n\n");
	// token_t *gold;
	init_parameters();
	reset_sync();
		
	printf("\n Coherence mode: %s,\n spandex config: %x\n", print_coh, tiled_app_cfg_000[0].spandex_reg);
	printf("num_devices = %d\n", num_devices);
	
	// }
	/* <<--print-params-->> */
	printf("  .num_tiles = %d\n", num_tiles);
	printf("  .tile_size = %d\n", tile_size);
	printf("  .rd_wr_enable = %d\n", rd_wr_enable);

	printf("CPU read offset: %d\n", output_buffer_offset[num_devices-1]);
	printf("CPU read sync offset: %d\n", accel_write_sync_spin_offset[num_devices-1]);
	printf("CPU write offset: %d\n", input_buffer_offset[0]);
	printf("CPU write sync offset: %d\n", accel_read_sync_spin_offset[0]);
	printf("At file %s func %s line %d\n", __FILE__, __func__, __LINE__);
	
	//Extra read for subsequent wtf-hits
	store_mem();

	int n_dev = 0;
	// printf("At file %s func %s line %d\n", __FILE__, __func__, __LINE__);
	printf("\n  ** START **\n");
	
	//Populate the pipeline
	int64_t total_time_start = get_counter();
	// for(int iteration = 0; iteration <num_exp; iteration++ ){
	if(regInv == 0){
		for(n_dev = 0; n_dev< num_devices;n_dev++){
			// printf("\n  ** Start ESP %d **\n", n_dev);
			esp_run(cfg_000+n_dev, NACC);
		}
		int stores = 0 , reads = 0;
		bool done = 0;
		int64_t accel_time_start = get_counter();
		if(pipeline){
			//Steady State and tail end
			int local_num_tiles = num_tiles;
			while(!done){
				// int32_t read_done = ;
				if(reads < local_num_tiles  && read_sync()){
					int64_t temp = get_counter();
					load_mem();
					int64_t temp2 = get_counter();
					t_cpu_write += (temp2-temp);
					update_load_sync();
					reads++;
				}
				// int32_t store_done = write_sync();
				if(stores < local_num_tiles && write_sync()){
					int64_t temp = get_counter();
					store_mem();
					int64_t temp2 = get_counter();
					t_cpu_read += (temp2-temp);
					update_store_sync();
					stores++;
				}
				done = (stores >= local_num_tiles) && (reads >= local_num_tiles);
			}
		}else{ //chaining
			int local_num_tiles = num_tiles;
			for(int tile = 0; tile < local_num_tiles; tile++){
				while(!read_sync());
				int64_t temp = get_counter();
				load_mem();
				int64_t temp2 = get_counter();
				t_cpu_write += (temp2-temp);
				update_load_sync();
				while(!write_sync()); //Wait for Output
				temp = get_counter();
				store_mem();
				temp2 = get_counter();
				t_cpu_read += (temp2-temp);
				update_store_sync();
			}
		}
		int64_t temp2 = get_counter();
		t_acc += (temp2-accel_time_start);
	}
	else{
		int local_num_tiles = num_tiles;
		for(int iteration = 0; iteration < local_num_tiles; iteration++ ){
			int64_t temp = get_counter();
			load_mem();
			int64_t temp2 = get_counter();
			t_cpu_write += (temp2-temp);
			update_load_sync();	//sets the flag
			int64_t accel_time_start = get_counter();
			//NOTE: TODO: UNCOMMENT
			for(n_dev = 0; n_dev< num_devices;n_dev++){
				// printf("\n  ** Start ESP %d **\n", n_dev);
				esp_run(cfg_000+n_dev, NACC);
			}
			temp2 = get_counter();
			t_acc += (temp2-accel_time_start);
			//NOTE: TODO: UNCOMMENT
			store_mem();
			// store_mem_hit();
			temp = get_counter();
			t_cpu_read += (temp-temp2);
			update_store_sync(); //zeros the flag
		}
	}
	int64_t temp2 = get_counter();
	t_total = temp2 - total_time_start;
	printf("\n  ** DONE **\n");
	printf("CPU Read Time: %ld\n", t_cpu_read);
	printf("CPU Write Time: %ld\n", t_cpu_write);
	printf("Accel Time: %ld\n", t_acc);
	printf("Total Time: %ld\n", t_total);
	// errors = validate_buffer(&buf[out_offset], gold);

	// free(gold);
	esp_free(buf);

	// if (!errors)
	// 	printf("+ Test PASSED\n");
	// else
	// 	printf("+ Test FAILED\n");


	return 0; //errors;
}
