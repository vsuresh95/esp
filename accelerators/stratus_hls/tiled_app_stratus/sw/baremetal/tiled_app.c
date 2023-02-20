/* Copyright (c) 2011-2021 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

// input /scratch/projects/bmishra3/spandex/esp_tiled_acc/socs/xilinx-vcu118-xcvu9p/restore.tcl.svcf

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#define QUAUX(X) #X
#define QU(X) QUAUX(X)
typedef int64_t token_t;

typedef union
{
  struct
  {
    unsigned int r_en    : 1;
    unsigned int r_op    : 1;
    unsigned int r_type  : 2;
    unsigned int r_cid   : 4;
    unsigned int w_en    : 1;
    unsigned int w_op    : 1;
    unsigned int w_type  : 2;
    unsigned int w_cid   : 4;
    unsigned int reserved: 16;
  };
  uint32_t spandex_reg;
} spandex_config_t;


#define NUM_DEVICES 1
// #define PRINT_DEBUG
// #define VALIDATE
// #define MEM_DUMP 1
#define NUM_TILES 1024
#define TILE_SIZE 1024
#define SLD_TILED_APP 0x033
#define DEV_NAME "sld,tiled_app_stratus"
#define SYNC_BITS 1
#define SYNC_VAR_SIZE 4

/* Coherence Modes */
#define COH_MODE 0
#define ESP

#ifdef ESP
// ESP COHERENCE PROTOCOLS
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
#if(COH_MODE == 3)
#define COHERENCE_MODE ACC_COH_NONE
#elif (COH_MODE == 2)
#define COHERENCE_MODE ACC_COH_LLC
#elif (COH_MODE == 1)
#define COHERENCE_MODE ACC_COH_RECALL
#else
#define COHERENCE_MODE ACC_COH_FULL
#endif
#else
//SPANDEX COHERENCE PROTOCOLS
#define COHERENCE_MODE ACC_COH_FULL
#if (COH_MODE == 3)
// Owner Prediction
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2262B82B
// spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_op = 1, .w_type = 1};
#elif (COH_MODE == 2)
// Write-through forwarding
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2062B02B
// spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_type = 1};
#elif (COH_MODE == 1)
// Baseline Spandex
#define READ_CODE 0x2002B30B
#define WRITE_CODE 0x0062B02B
// spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 1};
#else
// Fully Coherent MESI
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
// spandex_config_t spandex_config = {.spandex_reg = 0};
#endif
#endif

spandex_config_t spandex_config;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)



static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


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

// static void start_network_counter()
// {
//     uint64_t tile_id;
//     for (tile_id = 0; tile_id < SOC_COLS * SOC_ROWS; tile_id++)
//     {
//         volatile void* monitor_base = (volatile void*)(0x60090000 + 0x200 * tile_id);
//         volatile uint32_t* control = ((volatile uint32_t*)monitor_base) + 1; // ctrl_window_size_offset
//         *control = 0xffffffff;
//     }
// }

// static uint32_t get_network_counter(int x, int y, int plane)
// {
//     uint64_t tile_id = x + y * SOC_COLS;
//     volatile void* monitor_base = (volatile void*)(0x60090000 + 0x200 * tile_id);
//     volatile uint32_t* mon_register = ((volatile uint32_t*)monitor_base) + 4 + 22 + plane;
//     return *mon_register;
// }

static uint64_t get_counter() {
  uint64_t counter;
  asm volatile (
    "li t0, 0;"
    "csrr t0, mcycle;"
    "mv %0, t0"
    : "=r" ( counter )
    :
    : "t0"
  );

  return counter;
}

int32_t accel_read_sync_spin_offset[NUM_DEVICES];
int32_t accel_write_sync_spin_offset[NUM_DEVICES];
int32_t accel_read_sync_write_offset[NUM_DEVICES];
int32_t accel_write_sync_write_offset[NUM_DEVICES];
int32_t input_buffer_offset[NUM_DEVICES];
int32_t output_buffer_offset[NUM_DEVICES];

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


token_t *mem;
token_t **mem_list;
token_t *gold;
token_t *out;

/* <<--params-->> */
const int32_t num_tiles = NUM_TILES;//12;
const int32_t tile_size = TILE_SIZE;
const int32_t rd_wr_enable = 0;

int32_t num_dev = NUM_DEVICES;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;
static unsigned dev_mem_size;

static unsigned	int read_tile ;
static unsigned	int write_tile;

static unsigned coherence;

/* User defined registers */
/* <<--regs-->> */
#define TILED_APP_OUTPUT_TILE_START_OFFSET_REG 0x60
#define TILED_APP_INPUT_TILE_START_OFFSET_REG 0x5C
#define TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG 0x58
#define TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG 0x54
#define TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG 0x50
#define TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG 0x4C
#define TILED_APP_NUM_TILES_REG 0x48
#define TILED_APP_TILE_SIZE_REG 0x44
#define TILED_APP_RD_WR_ENABLE_REG 0x40

static inline uint32_t read_sync(){
	#ifdef ESP
	#if (COH_MODE==3 || COH_MODE==2 )
		// Flush because Non Coherent DMA/ LLC Coherent DMA
		start_flush = get_counter();
		esp_flush(coherence);
		stop_flush = get_counter();
		spin_flush += stop_flush-start_flush;
	#endif
	#endif
	// void* dst = (void*)(mem);
	void* dst = (void*)(mem + accel_read_sync_write_offset[0]);
	int64_t value_64 = 0;
	#if 1
	//!defined(ESP) && COH_MODE == 0
	// value_64 = mem[num_dev];
	value_64 = mem[accel_read_sync_write_offset[0]];
	#else
	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);
	#endif
	return (value_64 == 0);
}

static inline uint32_t write_sync(){
	#ifdef ESP
	#if 1
	//(COH_MODE==3 || COH_MODE==2 )
		// Flush because Non Coherent DMA/ LLC Coherent DMA
		start_flush = get_counter();
		esp_flush(coherence);
		stop_flush = get_counter();
		spin_flush += stop_flush-start_flush;
	#endif
	#endif
	// void* dst = (void*)(mem+num_dev);
	void* dst = (void*)(mem+accel_write_sync_write_offset[NUM_DEVICES-1]);
	int64_t value_64 = 0;
	#if !defined(ESP) && COH_MODE == 0
	// value_64 = mem[num_dev];
	value_64 = mem[accel_write_sync_write_offset[NUM_DEVICES-1]]; 
	#else
	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);
	#endif
	return (value_64 == 1);
}

static inline void update_load_sync(){
	#ifdef PRINT_DEBUG
	printf("Inside update load 1\n");
	#endif
	// void* dst = (void*)(mem);
	void* dst = (void*)(mem+accel_read_sync_spin_offset[0]);
	int64_t value_64 = 1;
	#if !defined(ESP) && COH_MODE == 0
//(COH_MODE == 0) && 
	// mem[0] = value_64;
	mem[accel_read_sync_spin_offset[0]] = value_64;
	#else
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE) 
		: 
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
	#endif

	#ifdef PRINT_DEBUG
	printf("Inside update load 2\n");
	#endif
	//Fence to push the write out from the write buffer
	asm volatile ("fence w, w");	
	stop_write = get_counter();
	intvl_write += stop_write - start_write;
	start_sync = stop_write ; //get_counter();
	#ifdef ESP
	#if (COH_MODE==3 || COH_MODE==2 )
		// Flush because Non Coherent DMA/ LLC Coherent DMA
		start_flush = get_counter();
		esp_flush(coherence);
		stop_flush = get_counter();
		intvl_flush += (stop_flush - start_flush);
		start_sync = stop_flush;
	#endif
	#endif
	#ifdef PRINT_DEBUG
	printf("Inside update load 3\n");
	#endif
}

static inline void update_store_sync(){
	// void* dst = (void*)(mem+num_dev);
	void* dst = (void*)(mem+accel_write_sync_spin_offset[NUM_DEVICES-1]);
	int64_t value_64 = 0; //Finished reading store_tile
	#if !defined(ESP) && COH_MODE == 0
//(COH_MODE == 0) && 
	// mem[0+num_dev] = value_64
	mem[accel_write_sync_spin_offset[NUM_DEVICES-1]] = value_64;;
	#else
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE) 
		: 
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
	#endif

	//Fence to push the write out from the write buffer
	asm volatile ("fence w, w");	
	stop_read = get_counter();
	intvl_read += stop_read - start_read;
	start_sync = stop_read ; //get_counter();
	#ifdef ESP
	#if (COH_MODE==3 || COH_MODE==2 )
	      // Flush because Non Coherent DMA/ LLC Coherent DMA
	      start_flush = get_counter();
	      esp_flush(coherence);
	      stop_flush = get_counter();
	      intvl_flush += (stop_flush - start_flush);
	      start_sync = stop_flush;
	#endif
	#endif
}

static inline void load_mem(){
	// void *dst = (void*)(&mem[64]);
	void *dst = (void*)(&mem[input_buffer_offset[0]]);
	#ifdef VALIDATE
	int64_t val_64 = read_tile;//23;
	#else
	static int64_t temp = 1;
	int64_t val_64 = (read_tile < num_tiles)?(temp++):0;
	#endif
	for (int j = 0; j < tile_size; j++){
		//int64_t value_64 = gold[(read_tile)*out_words_adj + j];
		#if (COH_MODE == 0) && !defined(ESP)
			// mem[64+j] = val_64;i
			mem[input_buffer_offset[0] + j] =val_64; 
		#else
		asm volatile (
			"mv t0, %0;"
			"mv t1, %1;"
			".word " QU(WRITE_CODE)
			: 
			: "r" (dst), "r" (val_64)
			: "t0", "t1", "memory"
		); //: "r" (dst), "r" (gold[(read_tile)*out_words_adj + j])

		dst += 8;
		#endif
#ifdef VALIDATE
		// val_64++;
#endif
	}
	asm volatile ("fence w, w");
	read_tile += 1;
}

static inline void store_mem(){
	// void *src = (void*)(mem+64+num_dev*tile_size);
	void *src = (void*)(mem+output_buffer_offset[NUM_DEVICES-1]);
	// out [i] = mem[j];
	int64_t out_val;
#if defined(VALIDATE) || defined(MEM_DUMP)
	static int64_t curTile = 0; //write_tile*tile_size;
#endif
	for (int j = 0; j < tile_size; j++){
		//int64_t value_64 = gold[(read_tile)*out_words_adj + j]
		#if ((COH_MODE == 0) && !defined(ESP))
			// mem[64+j] = val_64;i
			out_val = mem[output_buffer_offset[NUM_DEVICES-1] + j]; 
		#else
			asm volatile (
				"mv t0, %1;"
				".word " QU(READ_CODE) ";"
				"mv %0, t1"
				: "=r" (out_val)
				: "r" (src)
				: "t0", "t1", "memory"
			);//: "=r" (out[(write_tile) * out_words_adj + j])
			src += 8;
		#endif
#if defined(VALIDATE) || defined(MEM_DUMP)
		// out[curTile++] = out_val; //mem[tile_size + j];
#endif
	}

#if defined(VALIDATE) || defined(MEM_DUMP)
		printf("%d ", out_val);
		printf("\n");
#endif	
	asm volatile ("fence w, w");
	write_tile++;
}

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
	// in[offset] = 0;
	// in[1] = 0;i
	int64_t val_64 = 0;
	void * dst = (void*)(in);
	for (j = 0; j < buf_size; j++){
//		in[j] = 0;
//int64_t value_64 = gold[(read_tile)*out_words_adj + j];
		#if (COH_MODE == 0) && !defined(ESP)
//			mem[64+j] = val_64;
		in[j] = 0;
		#else
		asm volatile (
			"mv t0, %0;"
			"mv t1, %1;"
			".word " QU(WRITE_CODE)
			: 
			: "r" (dst), "r" (val_64)
			: "t0", "t1", "memory"
		); //: "r" (dst), "r" (gold[(read_tile)*out_words_adj + j])

		dst += 8;
		#endif
		// in[accel_read_sync_write_offset[0]] = 1;
		// in[accel_read_sync_write_offset[1]] = 1;
		// in[accel_read_sync_write_offset[2]] = 1;

	}
	// int seq = 0;
	// printf("Gold: ");
	//val_64 = 1;
	//for (i = 0; i < num_tiles; i++){
	//	for (j = 0; j < tile_size; j++){
	//		gold[i * out_words_adj + j] = (token_t) val_64++;// 2*((i) * in_words_adj + j + 5); //2*(seq++);
	//		out[i * out_words_adj + j] = 0;
	//	}
	//	
	//}
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
	int row_boundary = accel_write_sync_write_offset[0];
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

	//int cntr = 0;
	//int __i = (tile_size < 1024)? 0 : tile_size - loc_tilesize;
	//for(; __i < tile_size; __i++, cntr++){
	//	printf("\t%d", mem[64+__i]);
	//	if(cntr==15){
	//		printf("\n");
	//		cntr = -1;
	//	}
	//}
	//printf("\t||");
	//for(int nd = 0; nd < NUM_DEVICES; nd++){
	//__i = (tile_size < 1024)? 0 : tile_size - loc_tilesize;
	//for( cntr = 0; __i < tile_size; __i++, cntr++){
	//	printf("\t%d", mem[64+ (nd+1)*tile_size+__i]);
	//	if(cntr==15){
	//		printf("\n");
	//		cntr = -1;
	//	}
	//}
	//printf("\t||\t");
	//}
	//for(int __sync = 0; __sync <= num_dev; __sync++) printf("%d ", mem[__sync]);
	printf("\n");
}

inline static void print_coherence_mode(){
#ifdef ESP
// ESP COHERENCE PROTOCOLS
printf("ESP Coherence: ");
#if(COH_MODE == 3)
printf("ACC_COH_NONE\n");
#elif (COH_MODE == 2)
printf("ACC_COH_LLC\n");
#elif (COH_MODE == 1)
printf("ACC_COH_RECALL\n");
#else
printf("ACC_COH_FULL\n");
#endif
#else
//SPANDEX COHERENCE PROTOCOLS
printf("Spandex Coherence: (%x) : ", spandex_config.spandex_reg);
#if (COH_MODE == 3)
// Owner Prediction
printf("Owner Prediction\n");
#elif (COH_MODE == 2)
// Write-through forwarding
printf("Write-through forwarding\n");
#elif (COH_MODE == 1)
// Baseline Spandex ReqV
printf("Baseline Spandex ReqV\n");
#else
// Fully Coherent MESI
printf("MESI\n");
#endif
#endif
}

int main(int argc, char * argv[])
{
	// printf("Hello World 123\n");
	int32_t num_tiles = NUM_TILES;//12;
	int32_t tile_size = TILE_SIZE;

	#if 1
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned **ptable;
	unsigned ***ptable_list;
	unsigned errors = 0;
	// printf("Checkpoint 1\n");

	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = tile_size;
		out_words_adj = tile_size;
	} else {
		in_words_adj = round_up(tile_size, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(tile_size, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	// printf("Checkpoint 2\n");


	// Search for the device
	printf("Scanning device tree... uwu \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_TILED_APP, DEV_NAME);
	if (ndev == 0) {
		printf("tiled_app not found\n");
		return 0;
	}
	//if(ndev>1)ndev = NUM_DEVICES; //TODO:DELETE
	ndev = (NUM_DEVICES<ndev)? NUM_DEVICES : ndev;
	num_dev = ndev;

	#ifdef PRINT_DEBUG
	printf("Devices found:%d\n", ndev);
	#endif

	in_len = in_words_adj;//+64
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t)*num_dev;
	out_offset  = in_len;
	//mem_size = (out_offset * sizeof(token_t)) + out_size;
	mem_size = (NUM_DEVICES+1)*(TILE_SIZE + SYNC_VAR_SIZE)* sizeof(token_t);
	dev_mem_size = 2*(TILE_SIZE + SYNC_VAR_SIZE)* sizeof(token_t) ;
	// printf("Checkpoint 3\n");


	printf("num_tiles      =  %u\n",num_tiles     	); //num_tiles);
	printf("tile_size      =  %u\n",tile_size	    ); //num_tiles);
	// Allocate memory
	//gold = aligned_malloc(out_size*num_tiles+10);
	#ifdef VALIDATE
	out = aligned_malloc(out_size*num_tiles+10);
	#endif
	mem = aligned_malloc(mem_size);
	read_tile = 0;
		write_tile = 0;
		// Wait for completion
		done = 0;
		int done_prev = 0;
		int temp_rd_wr_enable = 0;
		int store_done_prev = 0;
		int load_turn = 1;
#ifdef PRINT_DEBUG
		// console_log_header();
#endif
		//load_mem(mem, gold, &read_tile); 



	//if(ndev>1){
	//	mem_list = aligned_malloc((ndev-1)*sizeof(token_t*)); 
	//	for(int __i = 0; __i<ndev-1; __i++)
	//		mem_list[__i] = aligned_malloc(mem_size+10);	
	//}
	for (n = 0; n < ndev; n++) {

		printf("**************** %s.%d ****************\n", DEV_NAME, n);

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


		print_coherence_mode();
		//printf("  gold buffer base-address = %p\n", gold);
		#ifdef VALIDATE
		printf("  out buffer base-address = %p\n", out);
		#endif
		printf("  memory buffer base-address = %p\n", mem);

		// // Alocate and populate page table
		// ptable = aligned_malloc(NCHUNK(dev_mem_size) * sizeof(unsigned *));
		// for (i = 0; i < NCHUNK(dev_mem_size); i++)
		// 	ptable[i] = (unsigned *) &mem[dev_mem_layout_offset  + i * (CHUNK_SIZE / sizeof(token_t))];
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		unsigned int dev_mem_layout_offset = (n*(tile_size + SYNC_VAR_SIZE)); //basically don't include input tile and input sync for previous accelerator
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];
		ptable_list[n] = ptable;

		printf("  ptable = %p\n", ptable);
		printf("  nchunk = %lu\n", NCHUNK(mem_size));

        	asm volatile ("fence w, rw");

		// If Spandex Caches
#ifndef ESP
		set_spandex_config_reg();
		#if(COH_MODE==3)
			spandex_config.w_cid = (n+2)%(NUM_DEVICES+1);
		#endif
		printf("Writing spandex :%d\n", spandex_config.spandex_reg);
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
#endif
	
	
		/* TODO: Restore full test once ESP caches are integrated */
		coherence = COHERENCE_MODE ; //ACC_COH_RECALL; //ACC_COH_FULL
	// #endif
		if(n == 0){
		printf("  --------------------\n");
		printf("  Generate input...\n");
		init_buf(mem, gold, out, mem_size/sizeof(token_t));
		//print_mem(2);
		//bmmishra3
		asm volatile ("fence w, rw");
		}
		// Pass common configuration parameters

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

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */

		// accel_read_sync_spin_offset[n]  = n*(2*tile_size + 2*SYNC_VAR_SIZE);
		// accel_write_sync_spin_offset[n] = n*(2*tile_size + 2*SYNC_VAR_SIZE) + tile_size + SYNC_VAR_SIZE;
		// accel_read_sync_write_offset[n] = n*(2*tile_size + 2*SYNC_VAR_SIZE);
		// accel_write_sync_write_offset[n]= n*(2*tile_size + 2*SYNC_VAR_SIZE) + tile_size + SYNC_VAR_SIZE;
		// input_buffer_offset[n]  		= n*(2*tile_size + 2*SYNC_VAR_SIZE) + SYNC_VAR_SIZE;
		// output_buffer_offset[n] 		= n*(2*tile_size + 2*SYNC_VAR_SIZE) + tile_size + 2*SYNC_VAR_SIZE;

		int32_t rel_output_buffer_offset = tile_size + 2*SYNC_VAR_SIZE;
		int32_t rel_input_buffer_offset = SYNC_VAR_SIZE;
		int32_t rel_accel_write_sync_write_offset = tile_size + SYNC_VAR_SIZE;
		int32_t rel_accel_read_sync_write_offset = 0;
		int32_t rel_accel_write_sync_spin_offset = rel_accel_write_sync_write_offset;//+2
		int32_t rel_accel_read_sync_spin_offset = rel_accel_read_sync_write_offset;	//+2

		//debug, sync
		accel_read_sync_spin_offset[n]  	= n*(tile_size + SYNC_VAR_SIZE) + rel_accel_read_sync_spin_offset;
		accel_write_sync_spin_offset[n] 	= n*(tile_size + SYNC_VAR_SIZE) + rel_accel_write_sync_spin_offset;
		accel_read_sync_write_offset[n] 	= n*(tile_size + SYNC_VAR_SIZE) + rel_accel_read_sync_write_offset;
		accel_write_sync_write_offset[n]	= n*(tile_size + SYNC_VAR_SIZE) + rel_accel_write_sync_write_offset;
		input_buffer_offset[n]  		= n*(tile_size + SYNC_VAR_SIZE) + rel_input_buffer_offset;
		output_buffer_offset[n] 		= n*(tile_size + SYNC_VAR_SIZE) + rel_output_buffer_offset;

		iowrite32(dev, TILED_APP_OUTPUT_TILE_START_OFFSET_REG , output_buffer_offset[n]);
		iowrite32(dev, TILED_APP_INPUT_TILE_START_OFFSET_REG  , input_buffer_offset[n]);
		iowrite32(dev, TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG, accel_write_sync_write_offset[n]);
		iowrite32(dev, TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG , accel_read_sync_write_offset[n]);
		iowrite32(dev, TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG  , accel_write_sync_spin_offset[n]); //
		iowrite32(dev, TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG   , accel_read_sync_spin_offset[n]);

		printf("Accel: %d\n", n);
		printf("TILED_APP_OUTPUT_TILE_START_OFFSET_REG : %d\n", output_buffer_offset[n]        );
		printf("TILED_APP_INPUT_TILE_START_OFFSET_REG  : %d\n", input_buffer_offset[n]         );
		printf("TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG: %d\n", accel_write_sync_write_offset[n]);
		printf("TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG : %d\n", accel_read_sync_write_offset[n]);
		printf("TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG  : %d\n", accel_write_sync_spin_offset[n]);
		printf("TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG   : %d\n", accel_read_sync_spin_offset[n] );
		printf("\n");

		// iowrite32(dev, TILED_APP_OUTPUT_TILE_START_OFFSET_REG , rel_output_buffer_offset);
		// iowrite32(dev, TILED_APP_INPUT_TILE_START_OFFSET_REG  , rel_input_buffer_offset);
		// iowrite32(dev, TILED_APP_OUTPUT_UPDATE_SYNC_OFFSET_REG, rel_accel_write_sync_write_offset);
		// iowrite32(dev, TILED_APP_INPUT_UPDATE_SYNC_OFFSET_REG , rel_accel_read_sync_write_offset);
		// iowrite32(dev, TILED_APP_OUTPUT_SPIN_SYNC_OFFSET_REG  , rel_accel_write_sync_spin_offset); //
		// iowrite32(dev, TILED_APP_INPUT_SPIN_SYNC_OFFSET_REG   , rel_accel_read_sync_spin_offset);
		iowrite32(dev, TILED_APP_NUM_TILES_REG, num_tiles);//ndev-n- +ndev-n-1
		iowrite32(dev, TILED_APP_TILE_SIZE_REG, tile_size);
		iowrite32(dev, TILED_APP_RD_WR_ENABLE_REG, n);//device number

		// Flush (customize coherence model here)
		esp_flush(coherence);

		// Start accelerators
		printf("  Start...\n");
		iowrite32(dev, CMD_REG, CMD_MASK_START);

	}

	printf("CPU read offset: %d\n", output_buffer_offset[NUM_DEVICES-1]);
	printf("CPU read sync offset: %d\n", accel_write_sync_spin_offset[NUM_DEVICES-1]);
	printf("CPU write offset: %d\n", input_buffer_offset[0]);
	printf("CPU write sync offset: %d\n", accel_read_sync_spin_offset[0]);


	dev = &espdevs[ndev-1];
		void* dst = (void*)(mem);
		// Load 1st Tile
//		load_mem();

//		update_load_sync();
//#ifdef PRINT_DEBUG
//		print_mem(2);
//#endif

		intvl_flush = 0;
		intvl_sync = 0;
		spin_flush = 0;
		intvl_write = 0;
		intvl_read = 0;
		intvl_write = 0;
		start_tiling = get_counter();
		start_sync = start_tiling;
		// start_tiling = start_write;
		while ( !done ) {
			//printf("readtile: %d writetile: %d\n", read_tile, write_tile);
#ifdef PRINT_DEBUG
			printf("Done:");
			for(int nd = 0; nd<ndev; nd++){
				dev = &espdevs[ndev-1];
				done = ioread32(dev, STATUS_REG);
				printf("%d ", (done&STATUS_MASK_DONE == STATUS_MASK_DONE));
			}
			printf("\n");
			print_mem(2);
#else
done = ioread32(dev, STATUS_REG);
#endif
			done &= STATUS_MASK_DONE;
			//esp_flush(coherence);

			int32_t store_done = write_sync();
			if(store_done==1 ){
			//	if(write_tile < NUM_TILES){
				stop_sync = get_counter();
				intvl_sync += stop_sync- start_sync - spin_flush;
				spin_flush = 0;
				start_read = get_counter();
				store_mem();
				update_store_sync();
				// stop_read = get_counter();
				// intvl_read += stop_read - start_read;
				#ifdef PRINT_DEBUG
				print_mem(0);
				#endif
				//} else {update_store_sync();
				//	print_mem(2);
				//}

			}
			if(done) break;
			int32_t read_done = read_sync();
			if(read_done==1){
				if(read_tile<num_tiles){
				stop_sync = get_counter();
				intvl_sync += stop_sync- start_sync - spin_flush;
				spin_flush = 0;
					start_write = get_counter();
					load_mem();
#ifdef PRINT_DEBUG
					print_mem(1);
					printf("Update load sync()\n");
#endif
					update_load_sync();
#ifdef PRINT_DEBUG
					printf("Done:Update load sync()\n");
					print_mem(2);
					printf("Done:Printmen\n");
#endif
					// stop_write = get_counter();
					// intvl_write += stop_write - start_write;
				}
				// intvl_read += stop_read - start_read;
			}
			
		}

		stop_tiling = get_counter();;
		intvl_tiling = stop_tiling - start_tiling;
		//What is this doing?
		for(int nd = 0; nd < ndev; nd++){
			dev = &espdevs[ndev-1];
			iowrite32(dev, CMD_REG, 0x0);
		}
		printf("  Done\n");
#ifdef MEM_DUMP
for(int temp_i = 0; temp_i < num_tiles*tile_size; temp_i++) printf("%d\n", out[temp_i]);
#endif
#ifdef VALIDATE
		printf("  validating...\n");

		/* Validation */
		errors = validate_buf(out, gold);
		if (errors)
			printf("  ... FAIL\n");
		else
			printf("  ... PASS\n");
#endif
		printf("  CPU Write Time: %lu\n", intvl_write		);
		printf("  Acc R/W   Time: %lu\n", intvl_sync		);
		printf("  CPU Read  Time: %lu\n", intvl_read		);
		// printf("  Spin 		Time: %lu\n", intvl_read		);;
		#ifdef ESP
		#if (COH_MODE==3 || COH_MODE==2)
			// Flush because Non Coherent DMA / LLC Coherent DMA
		printf("  Coh  Flush Time: %lu\n", intvl_flush		);
		#endif
		#endif
		printf("  Total Tile Time: %lu\n", intvl_tiling	);
		
		// print_mem(2);
	
	#ifdef VALIDATE
	aligned_free(out);
	#endif
	for(int dev_ = 0; dev_ < num_dev; dev_++) aligned_free(ptable[dev_]);
	aligned_free(ptable);
	aligned_free(mem);
	//aligned_free(gold);
	#endif

	return 0;
}
