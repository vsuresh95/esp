/* Copyright (c) 2011-2021 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

///////////////////////////////////////////////////////////////
// Helper unions
///////////////////////////////////////////////////////////////
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

typedef int64_t token_t;

///////////////////////////////////////////////////////////////
// Choosing the read, write code and coherence register config
///////////////////////////////////////////////////////////////
#define QUAUX(X) #X
#define QU(X) QUAUX(X)

#if (IS_ESP == 1)
// ESP COHERENCE PROTOCOLS
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
const spandex_config_t spandex_config = {.spandex_reg = 0};

#if (COH_MODE == 3)
const unsigned coherence = ACC_COH_NONE;
const char print_coh[] = "Non-Coherent DMA";
#elif (COH_MODE == 2)
const unsigned coherence = ACC_COH_LLC;
const char print_coh[] = "LLC-Coherent DMA";
#elif (COH_MODE == 1)
const unsigned coherence = ACC_COH_RECALL;
const char print_coh[] = "Coherent DMA";
#else
const unsigned coherence = ACC_COH_FULL;
const char print_coh[] = "Baseline MESI";
#endif

#else
//SPANDEX COHERENCE PROTOCOLS
const unsigned coherence = ACC_COH_FULL;
#if (COH_MODE == 3)
// Owner Prediction
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2262B82B
const spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_op = 1, .w_type = 1};
const char print_coh[] = "Owner Prediction";
#elif (COH_MODE == 2)
// Write-through forwarding
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2062B02B
const spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_type = 1};
const char print_coh[] = "Write-through forwarding";
#elif (COH_MODE == 1)
// Baseline Spandex
#define READ_CODE 0x2002B30B
#define WRITE_CODE 0x0062B02B
const spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 1};
const char print_coh[] = "Baseline Spandex (ReqV)";
#else
// Fully Coherent MESI
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
const spandex_config_t spandex_config = {.spandex_reg = 0};
const char print_coh[] = "Baseline Spandex";
#endif
#endif

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

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

///////////////////////////////////////////////////////////////
// Timer unions
///////////////////////////////////////////////////////////////
static uint64_t t_start = 0;
static uint64_t t_stop = 0;

uint64_t t_cpu_write;
uint64_t t_acc_read;
uint64_t t_acc_write;
uint64_t t_cpu_read;

static inline void start_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

static inline uint64_t end_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_stop)
		:
		: "t0"
	);

	return (t_stop - t_start);
}

#define SLD_SENSOR_DMA 0x050
#define DEV_NAME "sld,sensor_dma_stratus"

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define SENSOR_DMA_RD_SP_OFFSET_REG 0x58
#define SENSOR_DMA_RD_WR_ENABLE_REG 0x54
#define SENSOR_DMA_WR_SIZE_REG 0x50
#define SENSOR_DMA_WR_SP_OFFSET_REG 0x4c
#define SENSOR_DMA_RD_SIZE_REG 0x48
#define SENSOR_DMA_DST_OFFSET_REG 0x44
#define SENSOR_DMA_SRC_OFFSET_REG 0x40

int main(int argc, char * argv[])
{
	int i, j;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned **ptable;
	token_t *mem;
	token_t *gold;
	unsigned errors = 0;

    unsigned mem_words = 1 << LOG_LEN; 
    unsigned mem_size = 2 * mem_words * sizeof(token_t); 

	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_SENSOR_DMA, DEV_NAME);
	if (ndev == 0) {
		printf("sensor_dma not found\n");
		return 0;
	}

	dev = &espdevs[0];

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
	mem = (token_t *) aligned_malloc(mem_size);
	gold = mem + mem_words;
		
	// Alocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

    asm volatile ("fence rw, rw");

	t_cpu_write = 0;
	t_acc_read = 0;
	t_acc_write = 0;
	t_cpu_read = 0;

	printf("%s\n", print_coh);

	for (i = 0; i < ITERATIONS; i++)
	{
		// Pass common configuration parameters 
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 0);
		iowrite32(dev, DST_OFFSET_REG, 0);

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

  		void* dst = (void*)(mem);
		int64_t value_64;

		// Start CPU write
		start_counter();
    	for (j = 0; j < mem_words; j++, dst+=8)
    	{
			value_64 = ITERATIONS+j+1;
			write_mem (dst, value_64);
    	}
    	asm volatile ("fence w, w");
      	t_cpu_write += end_counter();

	    iowrite32(dev, SENSOR_DMA_RD_SP_OFFSET_REG, 0);
	    iowrite32(dev, SENSOR_DMA_RD_WR_ENABLE_REG, 0);
	    iowrite32(dev, SENSOR_DMA_RD_SIZE_REG, mem_words);
	    iowrite32(dev, SENSOR_DMA_SRC_OFFSET_REG, 0);

		// Start ACC read
		start_counter();
		iowrite32(dev, CMD_REG, CMD_MASK_START);

		// Wait for completion
		done = 0;
		while (!done) {
			done = ioread32(dev, STATUS_REG);
			done &= STATUS_MASK_DONE;
		}

		iowrite32(dev, CMD_REG, 0x0);
      	t_acc_read += end_counter();

	    iowrite32(dev, SENSOR_DMA_RD_WR_ENABLE_REG, 1);
	    iowrite32(dev, SENSOR_DMA_WR_SIZE_REG, mem_words);
	    iowrite32(dev, SENSOR_DMA_WR_SP_OFFSET_REG, 0);
	    iowrite32(dev, SENSOR_DMA_DST_OFFSET_REG, mem_words);

		// Start ACC write
		start_counter();
		iowrite32(dev, CMD_REG, CMD_MASK_START);

		// Wait for completion
		done = 0;
		while (!done) {
			done = ioread32(dev, STATUS_REG);
			done &= STATUS_MASK_DONE;
		}

		iowrite32(dev, CMD_REG, 0x0);
      	t_acc_write += end_counter();

  		dst = (void*)(gold);

		// Start CPU read
		start_counter();
 	   	for (j = 0; j < mem_words; j++, dst+=8)
 	   	{
			value_64 = read_mem(dst);
			if (value_64 != ITERATIONS+j+1) errors++;
 	   	}
      	t_cpu_read += end_counter();

		// printf("Errors = %d\", errors);
		errors = 0;
	}

	printf("CPU write = %lu\n", t_cpu_write/ITERATIONS);
	printf("ACC read = %lu\n", t_acc_read/ITERATIONS);
	printf("ACC write = %lu\n", t_acc_write/ITERATIONS);
	printf("CPU read = %lu\n", t_cpu_read/ITERATIONS);
	printf("Total time = %lu\n\n", (t_cpu_write + t_acc_read + t_acc_write + t_cpu_read)/ITERATIONS);
	
	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	while(1);

	return 0;
}
