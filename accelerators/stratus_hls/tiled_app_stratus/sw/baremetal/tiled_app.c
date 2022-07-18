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

typedef int64_t token_t;

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



// #define PRINT_DEBUG
// #define VALIDATE
// #define MEM_DUMP 1
#define NUM_TILES 1200
#define TILE_SIZE 1024
#define SLD_TILED_APP 0x033
#define DEV_NAME "sld,tiled_app_stratus"
#define SYNC_BITS 1

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
token_t *gold;
token_t *out;

/* <<--params-->> */
const int32_t num_tiles = NUM_TILES;//12;
const int32_t tile_size = TILE_SIZE;
const int32_t rd_wr_enable = 0;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

static unsigned	int read_tile ;
static unsigned	int write_tile;

static unsigned coherence;





/* User defined registers */
/* <<--regs-->> */
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
	void* dst = (void*)(mem+2*tile_size);
	int64_t value_64 = 0;
	asm volatile (
		"mv t0, %1;"
		".word 0x0002B30B;"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);
	int count = 0;
	while(count < 10) count++;
	return ((value_64 != 0)&&(value_64 != 1));
}

static inline void update_load_sync(){
	void* dst = (void*)(mem+2*tile_size);
	int64_t value_64 = 1;
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word 0x0062B02B"
		: 
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
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
}

static inline void load_mem(){
	void *dst = (void*)(mem);
	#ifdef VALIDATE
	static int64_t val_64 = 1;//23;
	#else
	int64_t val_64 = 123;
	#endif
	for (int j = 0; j < tile_size; j++){
		//int64_t value_64 = gold[(read_tile)*out_words_adj + j];
		asm volatile (
			"mv t0, %0;"
			"mv t1, %1;"
			".word 0x0062B02B"
			: 
			: "r" (dst), "r" (val_64)
			: "t0", "t1", "memory"
		); //: "r" (dst), "r" (gold[(read_tile)*out_words_adj + j])

		dst += 8;
#ifdef VALIDATE
		val_64++;
#endif
	}
	asm volatile ("fence rw, rw");
	read_tile += 1;
}

static inline void store_mem(){
	void *src = (void*)(mem+tile_size);
	// out [i] = mem[j];
	int64_t out_val;
#if defined(VALIDATE) || defined(MEM_DUMP)
	int64_t curTile = write_tile*tile_size;
#endif
	for (int j = 0; j < tile_size; j++){
		//int64_t value_64 = gold[(read_tile)*out_words_adj + j];
		asm volatile (
			"mv t0, %1;"
			".word 0x0002B30B;"
			"mv %0, t1"
			: "=r" (out_val)
			: "r" (src)
			: "t0", "t1", "memory"
		);//: "=r" (out[(write_tile) * out_words_adj + j])
		src += 8;
#if defined(VALIDATE) || defined(MEM_DUMP)
		out[curTile++] = out_val; //mem[tile_size + j];
#endif
	}
	asm volatile ("fence rw, rw");
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
			if (gold[i * out_words_adj + j] != out[i * out_words_adj + j]){
				printf("tile: %d loc:%d gold: %d -- found: %d \n",i, loc, gold[i * out_words_adj + j], out[i * out_words_adj + j]);
				errors++;
			}
			loc++;
		}

	return errors;
}


static void init_buf (token_t *in, token_t * gold, token_t* out)
{
	int i;
	int j;
	int offset = 2*tile_size;
	// in[offset] = 0;
	// in[1] = 0;
	for (j = 0; j < 2*tile_size+1; j++){
		in[j] = 0;
	}
	// int seq = 0;
	// printf("Gold: ");
	int64_t val_64 = 1;
	for (i = 0; i < num_tiles; i++){
		for (j = 0; j < tile_size; j++){
			gold[i * out_words_adj + j] = (token_t) val_64++;// 2*((i) * in_words_adj + j + 5); //2*(seq++);
			out[i * out_words_adj + j] = 0;
		}
		
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
	printf("Poll  Tile: %d\t||", write_tile);
	int cntr = 0;
	int __i = (tile_size < 1024)? 0 : tile_size - loc_tilesize;
	for(; __i < tile_size; __i++, cntr++){
		printf("\t%d", mem[__i]);
		if(cntr==15){
			printf("\n");
			cntr = -1;
		}
	}
	printf("\t||");
	__i = (tile_size < 1024)? 0 : tile_size - loc_tilesize;
	for( cntr = 0; __i < tile_size; __i++, cntr++){
		printf("\t%d", mem[tile_size+__i]);
		if(cntr==15){
			printf("\n");
			cntr = -1;
		}
	}
	printf("\t||\t%d\n", mem[2*tile_size]);
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

	in_len = in_words_adj+SYNC_BITS;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;

	// printf("Checkpoint 3\n");


	printf("num_tiles      =  %u\n",num_tiles     	); //num_tiles);
	printf("tile_size      =  %u\n",tile_size	    ); //num_tiles);

	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_TILED_APP, DEV_NAME);
	if (ndev == 0) {
		printf("tiled_app not found\n");
		return 0;
	}

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

		// Allocate memory
		gold = aligned_malloc(out_size*num_tiles+10);
		out = aligned_malloc(out_size*num_tiles+10);
		mem = aligned_malloc(mem_size+10);

		
		printf("  gold buffer base-address = %p\n", gold);
		printf("  out buffer base-address = %p\n", out);
		printf("  memory buffer base-address = %p\n", mem);

		// Alocate and populate page table
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

		printf("  ptable = %p\n", ptable);
		printf("  nchunk = %lu\n", NCHUNK(mem_size));

        asm volatile ("fence rw, rw");

		// If Spandex Caches
#ifndef ESP
			iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
#endif
		//#ifndef __riscv
		// 		for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
		// #else
		{
			/* TODO: Restore full test once ESP caches are integrated */
			coherence = COHERENCE_MODE ; //ACC_COH_RECALL; //ACC_COH_FULL
		// #endif
			printf("  --------------------\n");
			printf("  Generate input...\n");
			init_buf(mem, gold, out);
			//bmmishra3
        	asm volatile ("fence rw, rw");
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
			iowrite32(dev, TILED_APP_NUM_TILES_REG, num_tiles);
			iowrite32(dev, TILED_APP_TILE_SIZE_REG, tile_size);
			iowrite32(dev, TILED_APP_RD_WR_ENABLE_REG, rd_wr_enable);

			// Flush (customize coherence model here)
			esp_flush(coherence);

			// Start accelerators
			printf("  Start...\n");
			iowrite32(dev, CMD_REG, CMD_MASK_START);
			read_tile = 0;
			write_tile = 0;
			// Wait for completion
			done = 0;
			int done_prev = 0;
			int temp_rd_wr_enable = 0;
			int store_done_prev = 0;
        	int load_turn = 1;
#ifdef PRINT_DEBUG
			console_log_header();
#endif
			//load_mem(mem, gold, &read_tile); 
			intvl_flush = 0;
			intvl_sync = 0;
			spin_flush = 0;
			intvl_write = 0;
			void* dst = (void*)(mem);
			start_write = get_counter();
			// Load 1st Tile
			load_mem();
#ifdef PRINT_DEBUG
			print_mem(1);
#endif
			update_load_sync();
#ifdef PRINT_DEBUG
			print_mem(2);
#endif
			start_tiling = start_write;
			intvl_read = 0;
			while ( !done ) {
				done = ioread32(dev, STATUS_REG);
#ifdef PRINT_DEBUG
				printf("done:%d\n", done);
				print_mem(2);
#endif
				int32_t read_done = read_sync();
				if(read_done==1){
					stop_sync = get_counter();
					intvl_sync += stop_sync- start_sync - spin_flush;
					spin_flush = 0;
					start_read = get_counter();
					store_mem();
					stop_read = get_counter();
					intvl_read += stop_read - start_read;
#ifdef PRINT_DEBUG
					print_mem(0);
#endif
					if(read_tile<num_tiles){
						start_write = get_counter();
						load_mem();
#ifdef PRINT_DEBUG
						print_mem(1);
#endif
						update_load_sync();
#ifdef PRINT_DEBUG
						print_mem(2);
#endif
						// stop_write = get_counter();
						// intvl_write += stop_write - start_write;
					}
					// intvl_read += stop_read - start_read;
				}
				
				done &= STATUS_MASK_DONE;
			}
			//Fetch last tile
			if(write_tile < num_tiles) {
					start_read = get_counter();
					store_mem();
					stop_read = get_counter();
					intvl_read += stop_read - start_read;
#ifdef PRINT_DEBUG
					print_mem(0);
#endif
			}
			stop_tiling = get_counter();;
			intvl_tiling = stop_tiling - start_tiling;
			//What is this doing?
			iowrite32(dev, CMD_REG, 0x0);

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
			printf("  Tile 	Read Time: %lu\n", intvl_read		);
			printf("  Tile Write Time: %lu\n", intvl_write		);
			printf("  Acc   R/W  Time: %lu\n", intvl_sync		);
			#ifdef ESP
			#if (COH_MODE==3 || COH_MODE==2)
				// Flush because Non Coherent DMA / LLC Coherent DMA
			printf("  Coh  Flush Time: %lu\n", intvl_flush		);
			#endif
			#endif
			printf("  Total Tile Time: %lu\n", intvl_tiling	);
		}
		aligned_free(out);
		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);
	}
	#endif

	return 0;
}
