/* Copyright (c) 2011-2021 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#include "monitors.h"

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

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

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

uint64_t start_acc_write;
uint64_t stop_acc_write;
uint64_t intvl_acc_write;
uint64_t start_acc_read;
uint64_t stop_acc_read;
uint64_t intvl_acc_read;

#define ITERATIONS 1000
// #define ESP
#define COH_MODE 0
/* 3 - Owner Prediction, 2 - Write-through forwarding, 1 - Baseline Spandex (ReqV), 0 - Baseline Spandex (MESI) */
/* 3 - Non-Coherent DMA, 2 - LLC Coherent DMA, 1 - Coherent DMA, 0 - Fully Coherent MESI */

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

#define QUAUX(X) #X
#define QU(X) QUAUX(X)

#ifdef ESP

// ESP COHERENCE PROTOCOLS
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config;

#if (COH_MODE == 1)
#define ESP_COH ACC_COH_RECALL
const char print_coh[] = "Coherent DMA\n";
#else
#define ESP_COH ACC_COH_FULL
const char print_coh[] = "Baseline MESI\n";
#endif

#else

//SPANDEX COHERENCE PROTOCOLS

#if (COH_MODE == 3)
// Owner Prediction
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2262B82B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_op = 1, .w_type = 1};
const char print_coh[] = "Owner Prediction\n";

#elif (COH_MODE == 2)
// Write-through forwarding
#define READ_CODE 0x4002B30B
#define WRITE_CODE 0x2062B02B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 2, .w_en = 1, .w_type = 1};
const char print_coh[] = "Write-through forwarding\n";

#elif (COH_MODE == 1)
// Baseline Spandex
#define READ_CODE 0x2002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config = {.spandex_reg = 0, .r_en = 1, .r_type = 1};
const char print_coh[] = "Baseline Spandex (ReqV)\n";

#else
// Fully Coherent MESI
#define READ_CODE 0x0002B30B
#define WRITE_CODE 0x0062B02B
spandex_config_t spandex_config;
const char print_coh[] = "Baseline Spandex\n";

#endif

#define ESP_COH ACC_COH_FULL
#endif

esp_monitor_vals_t vals_start, vals_end, vals_diff;
esp_monitor_args_t mon_args;

static void start_mon()
{
	mon_args.read_mode = ESP_MON_READ_ALL;
    esp_monitor(mon_args, &vals_start);
}

static void end_mon()
{
    esp_monitor(mon_args, &vals_end);
    vals_diff = esp_monitor_diff(vals_start, vals_end);

    esp_monitor_print(mon_args, vals_diff);
}

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
	unsigned coherence;

    unsigned mem_words = 2048; 
    unsigned mem_size = mem_words*sizeof(token_t); 

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
	mem = (token_t *) aligned_malloc(2*mem_size);
	gold = mem + mem_words;
	printf("  memory = %p\n", mem);
	printf("  gold = %p\n", gold);
		
	// Alocate and populate page table
	ptable = aligned_malloc(NCHUNK(2*mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(2*mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(2*mem_size));

    asm volatile ("fence rw, rw");

	printf(print_coh);

	intvl_write = 0;
	intvl_acc_write = 0;
	intvl_read = 0;
	intvl_acc_read = 0;

	for (i = 0; i < ITERATIONS; i++)
	{
		/* TODO: Restore full test once ESP caches are integrated */
		coherence = ESP_COH;

		// Pass common configuration parameters 
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(2*mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 0);
		iowrite32(dev, DST_OFFSET_REG, 0);

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */
	    iowrite32(dev, SENSOR_DMA_RD_SP_OFFSET_REG, 2*mem_words);
	    iowrite32(dev, SENSOR_DMA_RD_WR_ENABLE_REG, 0);
	    iowrite32(dev, SENSOR_DMA_RD_SIZE_REG, mem_words);
	    iowrite32(dev, SENSOR_DMA_SRC_OFFSET_REG, 0);
	
        iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

  		void* dst = (void*)(mem);
		int64_t value_64 = 123;

        if (i == 5) start_mon();

      	start_write = get_counter();

    	for (j = 0; j < mem_words; j+=2)
    	{
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
	    		".word " QU(WRITE_CODE)
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);

			dst += 16;
    	}

      	stop_write = get_counter();
		intvl_write += stop_write - start_write;

        if (i == 5) end_mon();

		// Start accelerators
		iowrite32(dev, CMD_REG, CMD_MASK_START);

        if (i == 5) start_mon();

      	start_acc_read = get_counter();

		// Wait for completion
		done = 0;
		while (!done) {
			done = ioread32(dev, STATUS_REG);
			done &= STATUS_MASK_DONE;
		}

      	stop_acc_read = get_counter();
		intvl_acc_read += stop_acc_read - start_acc_read;

		iowrite32(dev, CMD_REG, 0x0);

        if (i == 5) end_mon();

	    iowrite32(dev, SENSOR_DMA_RD_WR_ENABLE_REG, 1);
	    iowrite32(dev, SENSOR_DMA_WR_SIZE_REG, mem_words);
	    iowrite32(dev, SENSOR_DMA_WR_SP_OFFSET_REG, 2*mem_words);
	    iowrite32(dev, SENSOR_DMA_DST_OFFSET_REG, mem_words);

		// Start accelerators
		iowrite32(dev, CMD_REG, CMD_MASK_START);

        if (i == 5) start_mon();

      	start_acc_write = get_counter();

		// Wait for completion
		done = 0;
		while (!done) {
			done = ioread32(dev, STATUS_REG);
			done &= STATUS_MASK_DONE;
		}

      	stop_acc_write = get_counter();
		intvl_acc_write += stop_acc_write - start_acc_write;

		iowrite32(dev, CMD_REG, 0x0);

        if (i == 5) end_mon();

  		dst = (void*)(gold);

        if (i == 5) start_mon();

      	start_read = get_counter();

 	   	for (j = 0; j < mem_words; j+=2)
 	   	{
			asm volatile (
				"mv t0, %1;"
	    		".word " QU(READ_CODE) ";"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			dst += 16;
 	   	}

      	stop_read = get_counter();
		intvl_read += stop_read - start_read;

        if (i == 5) end_mon();
	}

	printf("CPU write = %lu\n", intvl_write/ITERATIONS);
	printf("ACC read = %lu\n", intvl_acc_read/ITERATIONS);
	printf("ACC write = %lu\n", intvl_acc_write/ITERATIONS);
	printf("CPU read = %lu\n", intvl_read/ITERATIONS);
	printf("Total time = %lu\n", (intvl_write + intvl_acc_read + intvl_acc_write + intvl_read)/ITERATIONS);
	
	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	while(1);

	return 0;
}
